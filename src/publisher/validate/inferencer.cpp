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

Errorable<void> Inferencer::inference(CondVectType *conds) {
  for (const auto &gn : pgraph.nodes) {
    Errorable<void> res = std::visit(
        util::overloaded{
            [&](const graph::GraphVarnode &gvn) {
              return inferenceVarnode(gvn, conds);
            },
            [&](const graph::GraphPnode &gpn) {
              return inferencePnode(gpn, conds);
            }
        },
        *gn
    );
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
}

Errorable<void> Inferencer::inferencePnodeUserConds(
    const graph::GraphPnode &gp, CondVectType *conds
) {
  for (const auto &userBB : gp.userBbs) {

    // don't change conditions then we add id first time
    auto tmpConds = bbSolver.contains(userBB.id) ? conds : nullptr;
    auto res = addBBEqual(specvalues::BBOfPnode(gp.id), userBB.id, tmpConds);
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
    // don't change conditions then we add id first time
    auto tmpConds = bbSolver.contains(vt.declarationBB.id) ? conds : nullptr;
    auto res = addBBEqual(
        specvalues::BBOfVarnode(vg->id), vt.declarationBB.id, tmpConds
    );
    if (!res.has_value()) {
      return res;
    }

    tmpConds = conds;
    const ast::Id *sizeId = std::get_if<ast::Id>(&vt.size);
    if (sizeId != nullptr && !sizeSolver.contains(*sizeId)) {
      tmpConds = nullptr;
    }
    res = addSizeEqualSizeAndVarnode(vt.size, &gvn, tmpConds);
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
}

Errorable<void> Inferencer::inferenceUserConds(CondVectType *conds) {

  for (const auto &gn : pgraph.nodes) {
    Errorable<void> res = std::visit(
        util::overloaded{
            [&](const graph::GraphVarnode &gvn) {
              return inferenceVarnodeUserConds(gvn, conds);
            },
            [&](const graph::GraphPnode &gpn) {
              return inferencePnodeUserConds(gpn, conds);
            }
        },
        *gn
    );
    if (!res.has_value()) {
      return res;
    }
  }

  for (const auto &bbc : pgraph.bbUserConditions) {
    addBBDominate(bbc.a.id, bbc.b.id, conds);
  }

  return {};
}

} // namespace infer