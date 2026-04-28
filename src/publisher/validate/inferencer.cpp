#include "validate/inferencer.hh"
#include "inferencer.hh"

namespace infer {
Errorable<void> Inferencer::inferenceVarnode(
    const graph::GraphVarnode &gvn, CondVectType *conds
) {
  const graph::VarGraphNode *vg = std::get_if<graph::VarGraphNode>(&gvn);
  if (vg != nullptr) {
    auto def = vg->edges.def;
    if (def.has_value()) {
      graph::GraphPnode *gp = def.value();
      return addBBEqual(
          specvalues::BBOfVarnode(vg->id), specvalues::BBOfPnode(gp->id), conds
      );
    }
  }
  return {};
}

Errorable<void> Inferencer::addSizeEqualSizeAndVarnode(
    const ast::Size &sz, const graph::GraphVarnode *gvn, CondVectType *conds
) {
  const graph::VarGraphNode *vg = std::get_if<graph::VarGraphNode>(gvn);
  if (vg != nullptr) {
    solvers::SizeTerm szTerm = std::visit(
        util::overloaded{
            [&](const ast::Id &ident) -> solvers::SizeTerm { return ident; },
            [&](ghidra::int4 s) -> solvers::SizeTerm {
              return specvalues::ConcreateSize(s);
            }
        },
        sz
    );
    return addSizeEqual(szTerm, specvalues::SizeOfVarnode(vg->id), conds);
  }
  return {};
}

Errorable<void>
Inferencer::inferencePnode(const graph::GraphPnode &gp, CondVectType *conds) {

  for (const auto &[_arg_num, gvn] : gp.edges.inrefs) {
    const graph::VarGraphNode *vg = std::get_if<graph::VarGraphNode>(gvn);
    if (vg != nullptr) {
      auto res = addBBDominate(
          specvalues::BBOfVarnode(vg->id), specvalues::BBOfPnode(gp.id), conds
      );
      if (!res.has_value()) {
        return res;
      }
    }
  }

  if (gp.opTp.has_value()) {
    const ast::OpType &origOpTp = gp.opTp.value();
    ast::OpType freeOpTp = cntx.opTypePredefFactory.alphaUpdate(origOpTp);

    auto res = std::visit(
        util::overloaded{
            [&](const ast::InVarnodeConditionsArray &arr) -> Errorable<void> {
              for (size_t i = 0; i < arr.array.size(); i++) {
                if (gp.edges.inrefs.contains(i)) {
                  auto inres = addSizeEqualSizeAndVarnode(
                      arr.array[i].size, gp.edges.inrefs.at(i), conds
                  );
                  if (!inres.has_value()) {
                    return inres;
                  }
                }
              }
              return {};
            },
            [&](const ast::InVarnodeConditionsSpecial &) -> Errorable<void> {
              return {};
            }
        },
        freeOpTp.scheme.inVarnodeConds
    );

    const ast::Size *outSize = getSizeOutCond(freeOpTp.scheme.outVarnodeCond);
    if (outSize != nullptr) {
      if (gp.edges.output != nullptr) {
        addSizeEqualSizeAndVarnode(*outSize, gp.edges.output, conds);
      }
    }
  }
  return {};
}

Errorable<std::unique_ptr<Inferencer::CondVectType>>
Inferencer::inference(bool returnNewConds) {
  CondVectType *conds = nullptr;
  std::unique_ptr<CondVectType> vecPtr = std::make_unique<CondVectType>(0);

  if (returnNewConds) {
    conds = vecPtr.get();
  }

  for (const auto &[_id, pnode] : pgraph.pnodes) {
    auto res = inferencePnode(*pnode, conds);
    if (!res.has_value()) {
      return std::unexpected(res.error());
    }
  }

  for (const auto &[_id, varnode] : pgraph.varnodes) {
    auto res = inferenceVarnode(*varnode, conds);
    if (!res.has_value()) {
      return std::unexpected(res.error());
    }
  }
  return std::move(vecPtr);
}

Errorable<void> Inferencer::inferencePnodeUserConds(
    const graph::GraphPnode &gp, CondVectType *conds
) {
  for (const auto &userBB : gp.userBbs) {
    auto res = addBBEqual(specvalues::BBOfPnode(gp.id), userBB.id, conds);
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
}

Errorable<void> Inferencer::inferenceVarnodeUserConds(
    const graph::GraphVarnode &gvn, CondVectType *conds
) {
  const graph::VarGraphNode *vg = std::get_if<graph::VarGraphNode>(&gvn);

  for (const auto &vt : vg->userTypes) {
    auto res =
        addBBEqual(specvalues::BBOfVarnode(vg->id), vt.declarationBB.id, conds);
    if (!res.has_value()) {
      return res;
    }

    res = addSizeEqualSizeAndVarnode(vt.size, &gvn, conds);
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
}

Errorable<std::unique_ptr<Inferencer::CondVectType>>
Inferencer::inferenceUserConds(bool returnNewConds) {
  CondVectType *conds = nullptr;
  std::unique_ptr<CondVectType> vecPtr = std::make_unique<CondVectType>(0);

  if (returnNewConds) {
    conds = vecPtr.get();
  }

  for (const auto &[_id, pnode] : pgraph.pnodes) {
    auto res = inferencePnodeUserConds(*pnode, conds);
    if (!res.has_value()) {
      return std::unexpected(res.error());
    }
  }

  for (const auto &[_id, varnode] : pgraph.varnodes) {
    auto res = inferenceVarnodeUserConds(*varnode, conds);
    if (!res.has_value()) {
      return std::unexpected(res.error());
    }
  }
  return std::move(vecPtr);
}

} // namespace infer