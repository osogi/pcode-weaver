#include "rulecompile/compile_pattern.hh"

#include "common/util.hh"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace rulecompile {
namespace {

namespace compiled = pcodeweaver::compiled;

enum class KeyKind { Varnode, Pnode };

struct Key {
  KeyKind kind;
  const void *ptr;

  bool operator==(const Key &other) const {
    return kind == other.kind && ptr == other.ptr;
  }
};

struct KeyHash {
  std::size_t operator()(const Key &key) const {
    std::size_t seed = std::hash<const void *>{}(key.ptr);
    hash_combine(seed, static_cast<int>(key.kind));
    return seed;
  }
};

struct NextNode {
  Key key;
  compiled::StepSource source;
};

Key keyOf(const graph::GraphVarnode *gv) { return {KeyKind::Varnode, gv}; }
Key keyOf(const graph::GraphPnode *gp) { return {KeyKind::Pnode, gp}; }

const graph::GraphVarnode *asVarnode(Key key) {
  return static_cast<const graph::GraphVarnode *>(key.ptr);
}

const graph::GraphPnode *asPnode(Key key) {
  return static_cast<const graph::GraphPnode *>(key.ptr);
}

std::string nameOf(Key key) {
  if (key.kind == KeyKind::Pnode) {
    return graph::unpackGP(*asPnode(key))->id.getName();
  }
  return graph::toStr(asVarnode(key));
}

bool isGhost(Key key) {
  if (key.kind == KeyKind::Pnode) {
    return graph::unpackGP(*asPnode(key))->isGhost;
  }
  return std::visit([](const auto &node) { return node->isGhost; }, *asVarnode(key));
}

std::vector<NextNode> nextNodes(Key key, compiled::StepId from) {
  std::vector<NextNode> result;

  if (key.kind == KeyKind::Pnode) {
    const graph::OpGraphNode &pn = *graph::unpackGP(*asPnode(key));
    std::vector<std::pair<std::size_t, graph::GraphVarnode *>> inputs(
        pn.edges.inrefs.begin(), pn.edges.inrefs.end()
    );
    std::sort(inputs.begin(), inputs.end(), [](const auto &a, const auto &b) {
      return a.first < b.first;
    });
    for (const auto &[argNum, gv] : inputs) {
      if (gv == nullptr) {
        continue;
      }
      result.push_back(NextNode{
          .key = keyOf(gv),
          .source = compiled::StepSource{
              .kind = compiled::SourceKind::PnodeInput,
              .from = from,
              .inputIndex = static_cast<std::uint32_t>(argNum),
          },
      });
    }

    if (pn.edges.output != nullptr) {
      result.push_back(NextNode{
          .key = keyOf(pn.edges.output),
          .source = compiled::StepSource{
              .kind = compiled::SourceKind::PnodeOutput,
              .from = from,
              .inputIndex = 0,
          },
      });
    }
    return result;
  }

  const graph::VarnodeEdges &edges = graph::getEdges(asVarnode(key));
  if (edges.def.has_value() && edges.def.value() != nullptr) {
    result.push_back(NextNode{
        .key = keyOf(edges.def.value()),
        .source = compiled::StepSource{
            .kind = compiled::SourceKind::VarnodeDef,
            .from = from,
            .inputIndex = 0,
        },
    });
  }

  std::vector<graph::GraphPnode *> descends(
      edges.descend.begin(), edges.descend.end()
  );
  std::sort(descends.begin(), descends.end(), [](const auto *a, const auto *b) {
    return (graph::unpackGP(*a)->id <=> graph::unpackGP(*b)->id) < 0;
  });
  for (const graph::GraphPnode *gp : descends) {
    const graph::OpGraphNode &pn = *graph::unpackGP(*gp);
    for (const auto &[argNum, in] : pn.edges.inrefs) {
      if (static_cast<const void *>(in) == key.ptr) {
        result.push_back(NextNode{
            .key = keyOf(gp),
            .source = compiled::StepSource{
                .kind = compiled::SourceKind::VarnodeDescend,
                .from = from,
                .inputIndex = static_cast<std::uint32_t>(argNum),
            },
        });
        break;
      }
    }
  }

  return result;
}

compiled::MatchStep compileStep(Key key, compiled::StepSource source) {
  compiled::MatchStep step;
  step.source = source;

  if (key.kind == KeyKind::Pnode) {
    const graph::OpGraphNode &pn = *graph::unpackGP(*asPnode(key));
    step.kind = compiled::StepKind::Pnode;
    if (pn.opTp.has_value()) {
      step.hasOpCode = true;
      step.opCode = static_cast<std::int32_t>(pn.opTp->ghidraOpCode);
    }
    return step;
  }

  const graph::GraphVarnode *gv = asVarnode(key);
  std::visit(
      util::overloaded{
          [&](const graph::unq<graph::VarGraphNode> &) {
            step.kind = compiled::StepKind::AnyVarnode;
          },
          [&](const graph::unq<graph::ConstGraphNode> &cn) {
            step.kind = compiled::StepKind::Constant;
            step.constant = cn->origVarnode.value;
          },
          [&](const graph::unq<graph::EmptyGraphNode> &) {
            step.kind = compiled::StepKind::Empty;
          },
      },
      *gv
  );
  return step;
}

void collectGraphNodes(
    const graph::PcodeGraph &pgraph, std::vector<Key> &allNodes,
    std::vector<const graph::GraphPnode *> &typedPnodes,
    std::unordered_map<ast::Id, Key> &varnodeKeys,
    std::unordered_map<ast::Id, Key> &pnodeKeys
) {
  for (const auto &gn : pgraph.liveNodes()) {
    std::visit(
        util::overloaded{
            [&](const graph::GraphVarnode &gv) {
              Key key = keyOf(&gv);
              allNodes.push_back(key);
              if (const graph::VarGraphNode *vn =
                      get_if_uniq<graph::VarGraphNode>(&gv)) {
                varnodeKeys.emplace(vn->id, key);
              }
            },
            [&](const graph::GraphPnode &gp) {
              Key key = keyOf(&gp);
              allNodes.push_back(key);
              pnodeKeys.emplace(graph::unpackGP(gp)->id, key);
              if (graph::unpackGP(gp)->opTp.has_value()) {
                typedPnodes.push_back(&gp);
              }
            },
        },
        *gn
    );
  }
}

void collectSizeValueGhost(
    const specvalues::SpecValueSize &value,
    const std::unordered_map<ast::Id, Key> &varnodeKeys,
    std::unordered_set<Key, KeyHash> &referencedGhosts
) {
  if (const auto *size = std::get_if<specvalues::SizeOfVarnode>(&value)) {
    auto it = varnodeKeys.find(size->nodeId);
    if (it != varnodeKeys.end() && isGhost(it->second)) {
      referencedGhosts.insert(it->second);
    }
  }
}

void collectBBValueGhost(
    const specvalues::SpecValueBB &value,
    const std::unordered_map<ast::Id, Key> &varnodeKeys,
    const std::unordered_map<ast::Id, Key> &pnodeKeys,
    std::unordered_set<Key, KeyHash> &referencedGhosts
) {
  std::visit(
      util::overloaded{
          [&](const specvalues::BBOfVarnode &bb) {
            auto it = varnodeKeys.find(bb.nodeId);
            if (it != varnodeKeys.end() && isGhost(it->second)) {
              referencedGhosts.insert(it->second);
            }
          },
          [&](const specvalues::BBOfPnode &bb) {
            auto it = pnodeKeys.find(bb.nodeId);
            if (it != pnodeKeys.end() && isGhost(it->second)) {
              referencedGhosts.insert(it->second);
            }
          },
      },
      value
  );
}

void collectReferencedGhosts(
    const std::vector<speccond::SpecCondition> &conditions,
    const std::unordered_map<ast::Id, Key> &varnodeKeys,
    const std::unordered_map<ast::Id, Key> &pnodeKeys,
    std::unordered_set<Key, KeyHash> &referencedGhosts
) {
  for (const speccond::SpecCondition &condition : conditions) {
    std::visit(
        util::overloaded{
            [&](const speccond::SizeEqual &cond) {
              collectSizeValueGhost(cond.a, varnodeKeys, referencedGhosts);
              collectSizeValueGhost(cond.b, varnodeKeys, referencedGhosts);
            },
            [&](const speccond::BBEqual &cond) {
              collectBBValueGhost(cond.a, varnodeKeys, pnodeKeys, referencedGhosts);
              collectBBValueGhost(cond.b, varnodeKeys, pnodeKeys, referencedGhosts);
            },
            [&](const speccond::BBDominate &cond) {
              collectBBValueGhost(cond.a, varnodeKeys, pnodeKeys, referencedGhosts);
              collectBBValueGhost(cond.b, varnodeKeys, pnodeKeys, referencedGhosts);
            },
        },
        condition
    );
  }
}

void collectRequiredValueGhosts(
    const RuntimeValueRequirements &requirements,
    const std::unordered_map<ast::Id, Key> &varnodeKeys,
    const std::unordered_map<ast::Id, Key> &pnodeKeys,
    std::unordered_set<Key, KeyHash> &referencedGhosts
) {
  for (const auto &[id, key] : varnodeKeys) {
    if (isGhost(key) && (requirements.needsVarnodeSize(id) ||
                         requirements.needsVarnodeBB(id))) {
      referencedGhosts.insert(key);
    }
  }

  for (const auto &[id, key] : pnodeKeys) {
    if (isGhost(key) && requirements.needsPnodeBB(id)) {
      referencedGhosts.insert(key);
    }
  }
}

bool shouldCompile(
    Key key, const std::unordered_set<Key, KeyHash> &referencedGhosts
) {
  return !isGhost(key) || referencedGhosts.contains(key);
}

template <class MapType>
Errorable<compiled::StepId>
lookupStep(const MapType &map, const ast::Id &id, const char *kind) {
  auto it = map.find(id);
  if (it == map.end()) {
    return err(
        "Condition references " + std::string(kind) + " " + id.getName() +
        ", but it is not in the compiled pattern"
    );
  }
  return it->second;
}

struct CompiledValue {
  compiled::CheckValue value;
  std::optional<compiled::StepId> step;
};

Errorable<CompiledValue> compileSizeValue(
    const specvalues::SpecValueSize &value,
    const std::unordered_map<ast::Id, compiled::StepId> &varnodes
) {
  return std::visit(
      util::overloaded{
          [&](const specvalues::ConcreateSize &sz) -> Errorable<CompiledValue> {
            return CompiledValue{
                .value = compiled::CheckValue{
                    .kind = compiled::CheckValueKind::ConstantSize,
                    .step = 0,
                    .constant = sz.value,
                },
                .step = std::nullopt,
            };
          },
          [&](const specvalues::SizeOfVarnode &sz) -> Errorable<CompiledValue> {
            auto step = lookupStep(varnodes, sz.nodeId, "varnode");
            if (!step.has_value()) {
              return std::unexpected(step.error());
            }
            return CompiledValue{
                .value = compiled::CheckValue{
                    .kind = compiled::CheckValueKind::VarnodeSize,
                    .step = step.value(),
                    .constant = 0,
                },
                .step = step.value(),
            };
          },
      },
      value
  );
}

Errorable<CompiledValue> compileBBValue(
    const specvalues::SpecValueBB &value,
    const std::unordered_map<ast::Id, compiled::StepId> &varnodes,
    const std::unordered_map<ast::Id, compiled::StepId> &pnodes
) {
  return std::visit(
      util::overloaded{
          [&](const specvalues::BBOfVarnode &bb) -> Errorable<CompiledValue> {
            auto step = lookupStep(varnodes, bb.nodeId, "varnode");
            if (!step.has_value()) {
              return std::unexpected(step.error());
            }
            return CompiledValue{
                .value = compiled::CheckValue{
                    .kind = compiled::CheckValueKind::VarnodeBasicBlock,
                    .step = step.value(),
                    .constant = 0,
                },
                .step = step.value(),
            };
          },
          [&](const specvalues::BBOfPnode &bb) -> Errorable<CompiledValue> {
            auto step = lookupStep(pnodes, bb.nodeId, "pnode");
            if (!step.has_value()) {
              return std::unexpected(step.error());
            }
            return CompiledValue{
                .value = compiled::CheckValue{
                    .kind = compiled::CheckValueKind::PnodeBasicBlock,
                    .step = step.value(),
                    .constant = 0,
                },
                .step = step.value(),
            };
          },
      },
      value
  );
}

compiled::StepId attachPoint(
    std::optional<compiled::StepId> left, std::optional<compiled::StepId> right
) {
  if (!left.has_value()) {
    return right.value_or(0);
  }
  if (!right.has_value()) {
    return left.value();
  }
  return std::max(left.value(), right.value());
}

} // namespace

Errorable<compiled::PatternProgram> compilePattern(
    const graph::PcodeGraph &pgraph,
    const std::vector<speccond::SpecCondition> &runtimeChecks,
    const RuntimeValueRequirements &runtimeValueRequirements
) {
  std::vector<Key> allNodes;
  std::vector<const graph::GraphPnode *> typedPnodes;
  std::unordered_map<ast::Id, Key> varnodeKeys;
  std::unordered_map<ast::Id, Key> pnodeKeys;
  collectGraphNodes(pgraph, allNodes, typedPnodes, varnodeKeys, pnodeKeys);

  std::unordered_set<Key, KeyHash> referencedGhosts;
  collectReferencedGhosts(runtimeChecks, varnodeKeys, pnodeKeys, referencedGhosts);
  collectRequiredValueGhosts(
      runtimeValueRequirements, varnodeKeys, pnodeKeys, referencedGhosts
  );

  if (typedPnodes.empty()) {
    return err("Pattern compilation requires at least one pnode with op type");
  }

  std::erase_if(typedPnodes, [&](const graph::GraphPnode *gp) {
    return !shouldCompile(keyOf(gp), referencedGhosts);
  });
  if (typedPnodes.empty()) {
    return err("Pattern compilation requires a non-ignored pnode with op type");
  }

  std::sort(typedPnodes.begin(), typedPnodes.end(), [](const auto *a, const auto *b) {
    return (graph::unpackGP(*a)->id <=> graph::unpackGP(*b)->id) < 0;
  });

  compiled::PatternProgram program;
  std::unordered_map<Key, compiled::StepId, KeyHash> stepIds;
  std::unordered_map<Key, Key, KeyHash> parents;
  std::unordered_set<Key, KeyHash> queued;
  std::queue<Key> queue;

  Key root = keyOf(typedPnodes.front());
  stepIds[root] = 0;
  queued.insert(root);
  program.steps.push_back(compileStep(
      root,
      compiled::StepSource{
          .kind = compiled::SourceKind::RootPnode,
          .from = 0,
          .inputIndex = 0,
      }
  ));

  queue.push(root);
  while (!queue.empty()) {
    Key current = queue.front();
    queue.pop();
    compiled::StepId currentStep = stepIds.at(current);

    for (const NextNode &next : nextNodes(current, currentStep)) {
      if (!shouldCompile(next.key, referencedGhosts)) {
        continue;
      }
      auto parent = parents.find(current);
      if (parent != parents.end() && parent->second == next.key) {
        continue;
      }
      if (queued.contains(next.key)) {
        compiled::StepId expectedStep = stepIds.at(next.key);
        compiled::StepId checkStep = std::max(currentStep, expectedStep);
        program.steps[checkStep].edgeChecks.push_back(compiled::EdgeCheck{
            .source = next.source,
            .expected = expectedStep,
        });
        continue;
      }
      queued.insert(next.key);
      parents.emplace(next.key, current);
      stepIds[next.key] = static_cast<compiled::StepId>(program.steps.size());
      program.steps.push_back(compileStep(next.key, next.source));
      queue.push(next.key);
    }
  }

  std::vector<std::string> disconnected;
  for (Key key : allNodes) {
    if (shouldCompile(key, referencedGhosts) && !stepIds.contains(key)) {
      disconnected.push_back(nameOf(key));
    }
  }
  if (!disconnected.empty()) {
    std::sort(disconnected.begin(), disconnected.end());
    std::ostringstream msg;
    msg << "Pattern graph is disconnected; unsupported disconnected nodes:";
    for (const std::string &name : disconnected) {
      msg << " " << name;
    }
    return err(msg.str());
  }

  std::unordered_map<ast::Id, compiled::StepId> varnodeSteps;
  std::unordered_map<ast::Id, compiled::StepId> pnodeSteps;
  for (const auto &[key, step] : stepIds) {
    if (key.kind == KeyKind::Pnode) {
      pnodeSteps.emplace(graph::unpackGP(*asPnode(key))->id, step);
    } else if (const graph::VarGraphNode *vn =
                   get_if_uniq<graph::VarGraphNode>(asVarnode(key))) {
      varnodeSteps.emplace(vn->id, step);
    }
  }

  for (const speccond::SpecCondition &condition : runtimeChecks) {
    auto compiledCheck = std::visit(
        util::overloaded{
            [&](const speccond::SizeEqual &cond)
                -> Errorable<std::pair<compiled::Check, compiled::StepId>> {
              auto left = compileSizeValue(cond.a, varnodeSteps);
              if (!left.has_value()) {
                return std::unexpected(left.error());
              }
              auto right = compileSizeValue(cond.b, varnodeSteps);
              if (!right.has_value()) {
                return std::unexpected(right.error());
              }
              return std::pair{
                  compiled::Check{
                      .kind = compiled::CheckKind::SizeEqual,
                      .left = left->value,
                      .right = right->value,
                  },
                  attachPoint(left->step, right->step)};
            },
            [&](const speccond::BBEqual &cond)
                -> Errorable<std::pair<compiled::Check, compiled::StepId>> {
              auto left = compileBBValue(cond.a, varnodeSteps, pnodeSteps);
              if (!left.has_value()) {
                return std::unexpected(left.error());
              }
              auto right = compileBBValue(cond.b, varnodeSteps, pnodeSteps);
              if (!right.has_value()) {
                return std::unexpected(right.error());
              }
              return std::pair{
                  compiled::Check{
                      .kind = compiled::CheckKind::BasicBlockEqual,
                      .left = left->value,
                      .right = right->value,
                  },
                  attachPoint(left->step, right->step)};
            },
            [&](const speccond::BBDominate &cond)
                -> Errorable<std::pair<compiled::Check, compiled::StepId>> {
              auto left = compileBBValue(cond.a, varnodeSteps, pnodeSteps);
              if (!left.has_value()) {
                return std::unexpected(left.error());
              }
              auto right = compileBBValue(cond.b, varnodeSteps, pnodeSteps);
              if (!right.has_value()) {
                return std::unexpected(right.error());
              }
              return std::pair{
                  compiled::Check{
                      .kind = compiled::CheckKind::BasicBlockDominates,
                      .left = left->value,
                      .right = right->value,
                  },
                  attachPoint(left->step, right->step)};
            },
        },
        condition
    );

    if (!compiledCheck.has_value()) {
      return std::unexpected(compiledCheck.error());
    }
    program.steps[compiledCheck->second].checks.push_back(compiledCheck->first);
  }

  return program;
}

} // namespace rulecompile
