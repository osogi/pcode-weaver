#include "rulecompile/compile_action.hh"

#include "common/util.hh"

#include <algorithm>
#include <optional>
#include <unordered_map>

namespace rulecompile {
namespace {

namespace compiled = pcodeweaver::compiled;

struct ActionCompileContext {
  const graph::ActionPcodeGraph &actionGraph;
  const solvers::SizeSolver &sizeSolver;
  const PatternCompileResult &pattern;
  compiled::ActionProgram program;
  std::unordered_map<ast::Id, compiled::StepId> actionVarnodes;
  std::unordered_map<ast::Id, compiled::StepId> actionPnodes;
  compiled::StepId nextNodeId = 0;
};

template <class MapType>
Errorable<compiled::StepId>
lookupStep(const MapType &map, const ast::Id &id, const char *kind) {
  auto it = map.find(id);
  if (it == map.end()) {
    return err(
        "Action references " + std::string(kind) + " " + id.getName() +
        ", but it is not available in compiled action"
    );
  }
  return it->second;
}

Errorable<compiled::CheckValue> compileSize(
    const specvalues::SpecValueSize &value, const PatternCompileResult &pattern
) {
  return std::visit(
      util::overloaded{
          [](const specvalues::ConcreateSize &size)
              -> Errorable<compiled::CheckValue> {
            return compiled::CheckValue{
                .kind = compiled::CheckValueKind::ConstantSize,
                .step = 0,
                .constant = size.value,
            };
          },
          [&](const specvalues::SizeOfVarnode &size)
              -> Errorable<compiled::CheckValue> {
            auto step =
                lookupStep(pattern.varnodeSteps, size.nodeId, "varnode");
            if (!step.has_value()) {
              return std::unexpected(step.error());
            }
            return compiled::CheckValue{
                .kind = compiled::CheckValueKind::VarnodeSize,
                .step = step.value(),
                .constant = 0,
            };
          },
      },
      value
  );
}

std::optional<compiled::CheckValue> concreteOrPatternSize(
    const solvers::SizeSolver &sizeSolver, const ast::Id &id,
    const PatternCompileResult &pattern
) {
  std::optional<solvers::SizeTerm> term =
      sizeSolver.findoptConst(specvalues::SizeOfVarnode(id));
  if (!term.has_value()) {
    return std::nullopt;
  }

  const solvers::SizeValue *value = std::get_if<solvers::SizeValue>(&*term);
  if (value == nullptr) {
    return std::nullopt;
  }

  auto compiledSize = compileSize(*value, pattern);
  if (!compiledSize.has_value()) {
    return std::nullopt;
  }
  return compiledSize.value();
}

compiled::ActionNodeRef actionVarnode(compiled::StepId step) {
  return compiled::ActionNodeRef{
      .kind = compiled::ActionNodeKind::Varnode,
      .step = step,
      .constant = 0,
  };
}

compiled::ActionNodeRef actionPnode(compiled::StepId step) {
  return compiled::ActionNodeRef{
      .kind = compiled::ActionNodeKind::Pnode,
      .step = step,
      .constant = 0,
  };
}

Errorable<compiled::ActionNodeRef>
compileVarnodeRef(ActionCompileContext &ctx, const ast::VarnodeVar &var) {
  auto actionIt = ctx.actionVarnodes.find(var.id);
  if (actionIt != ctx.actionVarnodes.end()) {
    return actionVarnode(actionIt->second);
  }

  auto step = lookupStep(ctx.pattern.varnodeSteps, var.id, "varnode");
  if (!step.has_value()) {
    return std::unexpected(step.error());
  }
  return compiled::ActionNodeRef{
      .kind = compiled::ActionNodeKind::Varnode,
      .step = step.value(),
      .constant = 0,
  };
}

Errorable<compiled::ActionNodeRef>
compilePnodeRef(ActionCompileContext &ctx, const ast::PnodeVar &var) {
  auto actionIt = ctx.actionPnodes.find(var.id);
  if (actionIt != ctx.actionPnodes.end()) {
    return actionPnode(actionIt->second);
  }

  auto step = lookupStep(ctx.pattern.pnodeSteps, var.id, "pnode");
  if (!step.has_value()) {
    return std::unexpected(step.error());
  }
  return compiled::ActionNodeRef{
      .kind = compiled::ActionNodeKind::Pnode,
      .step = step.value(),
      .constant = 0,
  };
}

Errorable<compiled::ActionNodeRef> compileVarnodeTermRef(
    ActionCompileContext &ctx, const ast::VarnodeActionTerm &term
) {
  return std::visit(
      util::overloaded{
          [&](const ast::VarnodeVar &var) {
            return compileVarnodeRef(ctx, var);
          },
          [&](const ast::VarnodeSpecSize &spec) {
            return compileVarnodeRef(ctx, spec.newVar);
          },
          [](const ast::VarnodeEmpty &) -> Errorable<compiled::ActionNodeRef> {
            return compiled::ActionNodeRef{
                .kind = compiled::ActionNodeKind::Empty,
                .step = 0,
                .constant = 0,
            };
          },
          [](const ast::VarnodeConst &cn)
              -> Errorable<compiled::ActionNodeRef> {
            return compiled::ActionNodeRef{
                .kind = compiled::ActionNodeKind::Constant,
                .step = 0,
                .constant = cn.value,
            };
          },
      },
      term
  );
}

Errorable<compiled::ActionNodeRef> compilePnodeTermRef(
    ActionCompileContext &ctx, const ast::PnodeActionTerm &term
) {
  return std::visit(
      util::overloaded{
          [&](const ast::PnodeVar &var) { return compilePnodeRef(ctx, var); },
          [&](const ast::PnodeSpecTypeAndLoc &spec) {
            return compilePnodeRef(ctx, spec.newVar);
          },
      },
      term
  );
}

Errorable<compiled::ActionNodeRef>
compileVarnodeAction(ActionCompileContext &, const ast::VarnodeAction &);
Errorable<compiled::ActionNodeRef>
compilePnodeAction(ActionCompileContext &, const ast::PnodeAction &);

Errorable<compiled::ActionNodeRef> compileVarnodeAction(
    ActionCompileContext &ctx, const ast::VarnodeAction &action
) {
  return std::visit(
      util::overloaded{
          [&](const ast::VarnodeActionTerm &term) {
            return compileVarnodeTermRef(ctx, term);
          },
          [&](const ast::Box<ast::VarnodeSetAsPnodeOut> &box)
              -> Errorable<compiled::ActionNodeRef> {
            auto pnode = compilePnodeAction(ctx, box->pa);
            if (!pnode.has_value()) {
              return std::unexpected(pnode.error());
            }

            auto varnode = compileVarnodeTermRef(ctx, box->v);
            if (!varnode.has_value()) {
              return std::unexpected(varnode.error());
            }

            ctx.program.steps.push_back(
                compiled::ActionStep{
                    .kind = compiled::ActionStepKind::SetPnodeOutput,
                    .target = pnode.value(),
                    .value = varnode.value(),
                    .inputIndex = 0,
                }
            );
            return varnode.value();
          },
      },
      action
  );
}

Errorable<compiled::ActionNodeRef>
compilePnodeAction(ActionCompileContext &ctx, const ast::PnodeAction &action) {
  return std::visit(
      util::overloaded{
          [&](const ast::PnodeActionTerm &term) {
            return compilePnodeTermRef(ctx, term);
          },
          [&](const ast::Box<ast::PnodeSetNthArg> &box)
              -> Errorable<compiled::ActionNodeRef> {
            auto varnode = compileVarnodeAction(ctx, box->va);
            if (!varnode.has_value()) {
              return std::unexpected(varnode.error());
            }

            auto pnode = compilePnodeTermRef(ctx, box->p);
            if (!pnode.has_value()) {
              return std::unexpected(pnode.error());
            }

            ctx.program.steps.push_back(
                compiled::ActionStep{
                    .kind = compiled::ActionStepKind::SetPnodeInput,
                    .target = pnode.value(),
                    .value = varnode.value(),
                    .inputIndex = box->num,
                }
            );
            return pnode.value();
          },
      },
      action
  );
}

Errorable<void> compileNonDeleteAction(
    ActionCompileContext &ctx, const ast::RuleAction &action
) {
  return std::visit(
      util::overloaded{
          [&](const ast::VarnodeAction &varnode) -> Errorable<void> {
            auto result = compileVarnodeAction(ctx, varnode);
            if (!result.has_value()) {
              return std::unexpected(result.error());
            }
            return {};
          },
          [&](const ast::PnodeAction &pnode) -> Errorable<void> {
            auto result = compilePnodeAction(ctx, pnode);
            if (!result.has_value()) {
              return std::unexpected(result.error());
            }
            return {};
          },
          [](const ast::EmptyAction &) -> Errorable<void> { return {}; },
      },
      action
  );
}

Errorable<void>
compileDeleteAction(ActionCompileContext &ctx, const ast::RuleAction &action) {
  const ast::EmptyAction *empty = std::get_if<ast::EmptyAction>(&action);
  if (empty == nullptr) {
    return {};
  }

  return std::visit(
      util::overloaded{
          [&](const ast::EmptyActionDeletePnode &del) -> Errorable<void> {
            auto target = compilePnodeRef(ctx, del.targetPnode);
            if (!target.has_value()) {
              return std::unexpected(target.error());
            }
            ctx.program.steps.push_back(
                compiled::ActionStep{
                    .kind = compiled::ActionStepKind::DeletePnode,
                    .target = target.value(),
                }
            );
            return {};
          },
      },
      *empty
  );
}

void sortIds(std::vector<ast::Id> &ids) {
  std::sort(ids.begin(), ids.end(), [](const ast::Id &a, const ast::Id &b) {
    return (a <=> b) < 0;
  });
}

Errorable<void> compileCreateVarnodes(ActionCompileContext &ctx) {
  std::vector<ast::Id> ids = ctx.actionGraph.getNewVarnodeIds();
  sortIds(ids);

  for (const ast::Id &id : ids) {
    compiled::StepId step = ctx.nextNodeId++;
    ctx.actionVarnodes.emplace(id, step);

    compiled::ActionStep create{
        .kind = compiled::ActionStepKind::CreateVarnode,
        .target = actionVarnode(step),
    };
    std::optional<compiled::CheckValue> size =
        concreteOrPatternSize(ctx.sizeSolver, id, ctx.pattern);
    if (!size.has_value()) {
      return err("Can't get size value for new varnode " + id.getName());
    }
    create.hasSize = true;
    create.size = size.value();

    ctx.program.steps.push_back(create);
  }

  return {};
}

Errorable<void> compileCreatePnodes(ActionCompileContext &ctx) {
  std::vector<ast::Id> ids = ctx.actionGraph.getNewPnodeIds();
  sortIds(ids);

  for (const ast::Id &id : ids) {
    compiled::StepId step = ctx.nextNodeId++;
    ctx.actionPnodes.emplace(id, step);
  }

  for (const ast::Id &id : ids) {
    const graph::NewOpGraphNode *node = ctx.actionGraph.getNewPnode(id);
    if (node == nullptr) {
      return err("New pnode " + id.getName() + " is missing from action graph");
    }
    if (!node->opTp.has_value()) {
      return err("New pnode " + id.getName() + " has no operation type");
    }

    auto anchor = compilePnodeRef(ctx, ast::PnodeVar{node->oldVarId});
    if (!anchor.has_value()) {
      return std::unexpected(anchor.error());
    }

    compiled::StepId step = ctx.actionPnodes.at(id);
    ctx.program.steps.push_back(
        compiled::ActionStep{
            .kind = compiled::ActionStepKind::CreatePnode,
            .target = actionPnode(step),
            .value = anchor.value(),
            .inputIndex = 0,
            .insertBefore = node->isInsertBefore,
            .hasOpCode = true,
            .opCode = static_cast<std::int32_t>(node->opTp->ghidraOpCode),
        }
    );
  }

  return {};
}

} // namespace

Errorable<compiled::ActionProgram> compileAction(
    const std::vector<ast::RuleAction> &actions,
    const graph::ActionPcodeGraph &actionGraph,
    const solvers::SizeSolver &sizeSolver, const PatternCompileResult &pattern
) {
  ActionCompileContext ctx{
      .actionGraph = actionGraph,
      .sizeSolver = sizeSolver,
      .pattern = pattern,
      .nextNodeId = static_cast<compiled::StepId>(pattern.program.steps.size()),
  };

  auto res = compileCreateVarnodes(ctx);
  if (!res.has_value()) {
    return std::unexpected(res.error());
  }
  res = compileCreatePnodes(ctx);
  if (!res.has_value()) {
    return std::unexpected(res.error());
  }

  for (const ast::RuleAction &action : actions) {
    res = compileNonDeleteAction(ctx, action);
    if (!res.has_value()) {
      return std::unexpected(res.error());
    }
  }
  for (const ast::RuleAction &action : actions) {
    res = compileDeleteAction(ctx, action);
    if (!res.has_value()) {
      return std::unexpected(res.error());
    }
  }

  return std::move(ctx.program);
}

} // namespace rulecompile
