#include "validate/inferencer.hh"
#include "inferencer.hh"

using graph::unq;

namespace infer {
Errorable<void> Inferencer::inferenceVarnode(
    const graph::GraphVarnode &gvn, CondVectType *conds
) {
  const graph::VarGraphNode *vg = get_if_uniq<graph::VarGraphNode>(&gvn);
  if (vg != nullptr) {
    auto def = vg->edges.def;
    if (def.has_value()) {
      graph::OpGraphNode &og = *graph::unpackGP(*def.value());
      return addBBEqual(
          specvalues::BBOfVarnode(vg->id), specvalues::BBOfPnode(og.id), conds
      );
    }
  }
  return {};
}
Errorable<void> Inferencer::addSizeEqualSizeAndVarnode(
    const ast::Size &sz, const graph::GraphVarnode *gvn, CondVectType *conds
) {
  const graph::VarGraphNode *vg = get_if_uniq<graph::VarGraphNode>(gvn);
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

  const graph::OpGraphNode &og = *graph::unpackGP(gp);
  for (const auto &[_arg_num, gvn] : og.edges.inrefs) {
    const graph::VarGraphNode *vg = get_if_uniq<graph::VarGraphNode>(gvn);
    if (vg != nullptr) {
      auto res = addBBDominate(
          specvalues::BBOfVarnode(vg->id), specvalues::BBOfPnode(og.id), conds
      );
      if (!res.has_value()) {
        return res;
      }
    }
  }

  if (og.opTp.has_value()) {
    const ast::OpType &origOpTp = og.opTp.value();
    ast::OpType freeOpTp = cntx.opTypePredefFactory.alphaUpdate(origOpTp);

    auto res = std::visit(
        util::overloaded{
            [&](const ast::InVarnodeConditionsArray &arr) -> Errorable<void> {
              for (size_t i = 0; i < arr.array.size(); i++) {
                if (og.edges.inrefs.contains(i)) {
                  const ast::Size &opCnd = arr.array[i].size;
                  const ast::Id *opCndId = std::get_if<ast::Id>(&opCnd);

                  // don't add to condition if there isn't corresponding id
                  auto tmpConds = conds;
                  if (opCndId != nullptr && !sizeSolver.contains(*opCndId)) {
                    tmpConds = nullptr;
                  }

                  auto inres = addSizeEqualSizeAndVarnode(
                      opCnd, og.edges.inrefs.at(i), tmpConds
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
    if (!res.has_value()) {
      return res;
    }

    const ast::Size *outSize = getSizeOutCond(freeOpTp.scheme.outVarnodeCond);
    if (outSize != nullptr) {
      if (og.edges.output != nullptr) {

        const ast::Id *opCndId = std::get_if<ast::Id>(outSize);

        // don't add to condition if there isn't corresponding id
        auto tmpConds = conds;
        if (opCndId != nullptr && !sizeSolver.contains(*opCndId)) {
          tmpConds = nullptr;
        }
        auto outRes =
            addSizeEqualSizeAndVarnode(*outSize, og.edges.output, tmpConds);
        if (!outRes.has_value()) {
          return outRes;
        }
      }
    }
  }
  return {};
}
Errorable<void>
Inferencer::inference(const graph::PcodeGraph &pgraph, CondVectType *conds) {
  for (const auto &gn : pgraph.liveNodes()) {
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
  const graph::OpGraphNode &og = *graph::unpackGP(gp);

  for (const auto &userBB : og.userBbs) {

    // don't change conditions then we add id first time
    auto tmpConds = bbSolver.contains(userBB.id) ? conds : nullptr;
    auto res = addBBEqual(specvalues::BBOfPnode(og.id), userBB.id, tmpConds);
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
}
Errorable<void> Inferencer::inferenceVarnodeUserConds(
    const graph::GraphVarnode &gvn, CondVectType *conds
) {
  const graph::VarGraphNode *vg = get_if_uniq<graph::VarGraphNode>(&gvn);
  if (vg == nullptr) {
    return {};
  }

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
Errorable<void> Inferencer::inference(
    const graph::ActionPcodeGraph &pgraph, CondVectType *conds
) {
  auto res = inference(static_cast<const graph::PcodeGraph &>(pgraph), conds);
  if (!res.has_value()) {
    return res;
  }

  for (const auto &[id, nvg] : pgraph.newVarnodes) {
    graph::GraphVarnode *gn = pgraph.varnodes.at(id);
    for (const ast::Size &sz : nvg->specSizes) {
      res = addSizeEqualSizeAndVarnode(sz, gn, conds);
      if (!res.has_value()) {
        return res;
      }
    }
  }

  return {};
}
Errorable<void> Inferencer::inferenceUserConds(
    const graph::PcodeGraph &pgraph, CondVectType *conds
) {

  for (const auto &gn : pgraph.liveNodes()) {
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
    auto res = addBBDominate(bbc.a.id, bbc.b.id, conds);
    if (!res.has_value()) {
      return res;
    }
  }

  return {};
}

} // namespace infer
