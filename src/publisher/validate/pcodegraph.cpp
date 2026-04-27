#pragma once

#include "validate/pcodegraph.hh"
#include "pcodegraph.hh"

namespace graph {

VarnodeEdges &getEdges(GraphVarnode *gv) {
  return std::visit(
      util::overloaded{
          [](VarGraphNode &v) -> VarnodeEdges & { return v.edges; },
          [](ConstGraphNode &v) -> VarnodeEdges & { return v.edges; },
          [](EmptyGraphNode &v) -> VarnodeEdges & { return v.edges; },
      },
      *gv
  );
}

std::string toStr(const GraphVarnode *gv) {
  return std::visit(
      util::overloaded{
          [](const VarGraphNode &v) -> std::string { return v.id.getName(); },
          [](const ConstGraphNode &v) -> std::string {
            return "#" + std::to_string(v.origVarnode.value);
          },
          [](const EmptyGraphNode &v) -> std::string { return "EMPTY"; },
      },
      *gv
  );
}

bool isEmpty(const GraphVarnode *gv) {
  return std::holds_alternative<EmptyGraphNode>(*gv);
}

size_t PnodeEdges::updateMaxArgNum(bool onlyAction) {
  size_t maxArg = 0;
  for (auto &[argNum, gv] : inrefs) {
    if (!isEmpty(gv)) {
      if (maxArg < argNum) {
        maxArg = argNum;
      }
    }
  }

  if (!onlyAction) {
    maxAgrNumPattern = maxArg;
  }
  maxAgrNumAction = maxArg;

  return maxArg;
}

GraphVarnode *PcodeGraph::uniqAddToNodes(const GraphVarnode &gvn) {
  std::unique_ptr<GraphNode> gn_ptr = std::make_unique<GraphNode>(gvn);
  GraphVarnode *gvn_ptr = &std::get<GraphVarnode>(*gn_ptr);

  nodes.push_back(std::move(gn_ptr));
  return gvn_ptr;
}

GraphPnode *PcodeGraph::uniqAddToNodes(const OpGraphNode &pn) {
  std::unique_ptr<GraphNode> gn_ptr =
      std::make_unique<GraphNode>(GraphPnode(pn));
  GraphPnode *gpn_ptr = &std::get<GraphPnode>(*gn_ptr);

  nodes.push_back(std::move(gn_ptr));
  return gpn_ptr;
}

// returns the guaranteed VarGraphNode
GraphVarnode *PcodeGraph::findOrCreateVarnode(const ast::Id &id) {
  auto it = varnodes.find(id);
  if (it != varnodes.end()) {
    return it->second;
  } else {
    VarGraphNode vn(id);
    vn.systemType.size = cntx.sizeVarFactory.createId();
    vn.systemType.declarationBB =
        ast::BasicBlockVar{cntx.basicBlockVarFactory.createId()};

    varnodes[id] = uniqAddToNodes(vn);
    return varnodes[id];
  }
}

// returns the guaranteed OpGraphNode
GraphPnode *PcodeGraph::findOrCreatePnode(const ast::Id &id) {
  auto it = pnodes.find(id);
  if (it != pnodes.end()) {
    return it->second;
  } else {
    OpGraphNode pn(id);
    pn.systemBb = ast::BasicBlockVar{cntx.basicBlockVarFactory.createId()};

    pnodes[id] = uniqAddToNodes(pn);
    return pnodes[id];
  }
}
Errorable<void> PcodeGraph::validateVarnode(const GraphVarnode &gvn) {
  return std::visit(
      util::overloaded{
          [&](const VarGraphNode &var) {
            if (var.edges.def.has_value() && var.edges.def.value() == nullptr) {
              return err(
                  "Varnode " + var.id.getName() + " hasn't defined pnode"
              );
            }
            return;
          },
          [&](const ConstGraphNode &cnst) {
            if (cnst.edges.def.has_value() &&
                cnst.edges.def.value() != nullptr) {
              return err(
                  "Const varnode " + std::to_string(cnst.origVarnode.value) +
                  " defined by pnode " + cnst.edges.def.value()->id.getName()
              );
            }
            return;
          },
          [&](const EmptyGraphNode &cnst) { return; },
      },
      gvn
  );
}

Errorable<void> PcodeGraph::validatePnodeInSpecial(
    const GraphPnode &gpn, const ast::InVarnodeConditionsSpecial &spec
) {
  size_t patMaxArg = gpn.edges.maxAgrNumPattern;
  size_t actMaxArg = gpn.edges.maxAgrNumAction;

  size_t maxArgNum = std::min(patMaxArg, actMaxArg);
  for (const auto &[argNum, gv] : gpn.edges.inrefs) {
    if (argNum <= maxArgNum) {
      if (isEmpty(gv)) {
        return err(
            "Pnode " + gpn.id.getName() + " has EMPTY as " +
            std::to_string(argNum) +
            " arg. Not expected for all args <= " + std::to_string(argNum)
        );
      }
    }
  }

  if (patMaxArg < actMaxArg) {
    // added new args
    for (size_t i = patMaxArg + 1; i <= actMaxArg; i++) {
      std::string instead = "";
      if (gpn.edges.inrefs.contains(i)) {
        const GraphVarnode *gv = gpn.edges.inrefs.at(i);
        if (!isEmpty(gv)) {
          instead = toStr(gv);
        }
      } else {
        instead = "UNDEFINED";
      }

      if (instead.size() != 0) {
        return err(
            "Expected all args for pnode (" + gpn.id.getName() + ") from " +
            std::to_string(patMaxArg + 1) + " to " + std::to_string(actMaxArg) +
            " will be explicity and non-EMPTY. But got instead " + instead +
            " as " + std::to_string(i) + "arg"
        );
      }
    }
  } else {
    // removed old args
    for (size_t i = actMaxArg + 1; i <= patMaxArg; i++) {
      std::string instead = "";
      if (gpn.edges.inrefs.contains(i)) {
        const GraphVarnode *gv = gpn.edges.inrefs.at(i);
        if (isEmpty(gv)) {
          instead = toStr(gv);
        }
      } else {
        instead = "UNDEFINED";
      }

      if (instead.size() != 0) {
        return err(
            "Expected all args for pnode (" + gpn.id.getName() + ") from " +
            std::to_string(actMaxArg + 1) + " to " + std::to_string(patMaxArg) +
            " will be explicity and EMPTY. But got instead " + instead +
            " as " + std::to_string(i) + "arg"
        );
      }
    }
  }

  return;
}

Errorable<void> PcodeGraph::validatePnodeInArray(
    const GraphPnode &gpn, const ast::InVarnodeConditionsArray &arr
) {
  for (auto &[argNum, gv] : gpn.edges.inrefs) {

    if (isEmpty(gv)) {
      if (argNum < arr.array.size()) {
        return err(
            "Unexpected " + toStr(gv) + " as " + std::to_string(argNum) +
            " arg of pnode " + gpn.id.getName() + "expected non-Empty"
        );
      }
    } else {
      if (argNum >= arr.array.size()) {
        return err(
            "Unexpected " + toStr(gv) + " as " + std::to_string(argNum) +
            " arg of pnode " + gpn.id.getName() + "expected Empty"
        );
      }
    }
  }
}

Errorable<void> PcodeGraph::validatePnodeOut(
    const GraphPnode &gpn, const ast::OutVarnodeCondition &outCond
) {
  if (gpn.edges.output != nullptr) {
    if (isEmpty(gpn.edges.output)) {
      if (std::holds_alternative<ast::OutVarnodeConditionDefault>(outCond)) {
        return err(
            "Expected see non-empty output of pnode " + gpn.id.getName()
        );
      }
    } else {
      if (std::holds_alternative<ast::OutVarnodeConditionNoOut>(outCond)) {
        return err(
            "Expected see empty output of pnode " + gpn.id.getName() +
            ", but get " + toStr(gpn.edges.output)
        );
      }
    }
  }
  return;
}

Errorable<void> PcodeGraph::validatePnode(const GraphPnode &gpn) {
  if (gpn.opTp.has_value()) {
    const ast::OpType &opTp = gpn.opTp.value();
    auto res = std::visit(
        util::overloaded{
            [&](const ast::InVarnodeConditionsArray &arr) {
              return validatePnodeInArray(gpn, arr);
            },
            [&](const ast::InVarnodeConditionsSpecial &spec) {
              return validatePnodeInSpecial(gpn, spec);
            },
        },
        opTp.scheme.inVarnodeConds
    );
    if (!res.has_value()) {
      return res;
    }
  }
}

// Public

GraphVarnode *PcodeGraph::addVarnodeTerm(const ast::VarnodeTerm &vt) {
  return std::visit(
      util::overloaded{
          [this](const ast::VarnodeVar &vn) {
            return findOrCreateVarnode(vn.id);
          },
          [this](const ast::VarnodeVarWithType &vn) {
            GraphVarnode *gvn = findOrCreateVarnode(vn.var.id);
            VarGraphNode *vgn = get_if_force<VarGraphNode>(gvn);
            vgn->userTypes.push_back(vn.vntype);
            return gvn;
          },
          [this](const ast::VarnodeEmpty &orig) {
            return uniqAddToNodes(EmptyGraphNode{orig});
          },
          [this](const ast::VarnodeConst &orig) {
            return uniqAddToNodes(ConstGraphNode{orig});
          }
      },
      vt
  );
}

Errorable<GraphPnode *> PcodeGraph::addPnodeTerm(const ast::PnodeTerm &pt) {
  return std::visit(
      util::overloaded{
          [this](const ast::PnodeVar &pv) -> Errorable<GraphPnode *> {
            return findOrCreatePnode(pv.id);
          },
          [this](const ast::PnodeVarWithType &pvt) -> Errorable<GraphPnode *> {
            GraphPnode *gpn = findOrCreatePnode(pvt.var.id);
            OpGraphNode *pgn = static_cast<OpGraphNode *>(gpn);
            pgn->userBbs.push_back(pvt.ptype.bb);

            if (pgn->opTp.has_value()) {
              if (pgn->opTp.value() != pvt.ptype.optype) {
                return err(
                    "For pnode " + pvt.var.id.getName() +
                    "setted two differ operations"
                );
              }
            } else {
              pgn->opTp = pvt.ptype.optype;
            }
            return gpn;
          },
      },
      pt
  );
}

Errorable<GraphVarnode *>
PcodeGraph::addVarnodePattern(const ast::VarnodePattern &vp) {
  return std::visit(
      util::overloaded{
          [this](const ast::VarnodeTerm &v) -> Errorable<GraphVarnode *> {
            return addVarnodeTerm(v);
          },
          [this](const ast::Box<ast::VarnodeDefedBy> &b) {
            const ast::VarnodeDefedBy &defedBy = *b;
            return addPnodePattern(defedBy.pp)
                .and_then([&](GraphPnode *gp) -> Errorable<GraphVarnode *> {
                  GraphVarnode *gv = addVarnodeTerm(defedBy.v);
                  auto &pOut = gp->edges.output;
                  auto &vIn = getEdges(gv).def;

                  if (pOut != nullptr) {
                    return err(
                        "Pnode " + gp->id.getName() + " already had output (" +
                        toStr(pOut) + ") during adding " + toStr(gv) +
                        " as new one"
                    );
                  }
                  pOut = gv;

                  if (vIn.has_value()) {
                    return err(
                        "Varnode " + toStr(gv) + " already had def pnode (" +
                        vIn.value()->id.getName() + ") during adding " +
                        gp->id.getName() + " as new one"
                    );
                  }
                  vIn = gp;

                  return gv;
                });
          },
      },
      vp
  );
}

Errorable<GraphPnode *>
PcodeGraph::addPnodePattern(const ast::PnodePattern &pp) {
  return std::visit(
      util::overloaded{
          [this](const ast::PnodeTerm &p) { return addPnodeTerm(p); },
          [this](const ast::Box<ast::PnodeThatTakeAsNthArg> &b) {
            const ast::PnodeThatTakeAsNthArg &nArg = *b;
            return addVarnodePattern(nArg.vp).and_then([&](GraphVarnode *vn) {
              return addPnodeTerm(nArg.p).and_then(
                  [&](GraphPnode *pn) -> Errorable<GraphPnode *> {
                    uint32_t n = nArg.num;
                    if (pn->edges.inrefs.contains(n)) {
                      return err(
                          "Pnode " + pn->id.getName() + " already had " +
                          toStr(pn->edges.inrefs[n]) + " as arg number " +
                          std::to_string(n) + " during adding " + toStr(vn) +
                          "as new one"
                      );
                    };

                    pn->edges.inrefs[n] = vn;
                    getEdges(vn).descend.push_back(pn);
                    return pn;
                  }
              );
            });
          },
      },
      pp
  );
}

const ast::BasicBlockVar &
PcodeGraph::addBBPattern(const ast::BasicBlockPattern &bbp) {
  return std::visit(
      util::overloaded{
          [this](const ast::BasicBlockVar &bbv) { return bbv; },
          [this](const ast::Box<ast::BasicBlockDominatedBy> &b) {
            const ast::BasicBlockDominatedBy &bbdb = *b;
            const ast::BasicBlockVar &first = addBBPattern(bbdb.bbp);
            const ast::BasicBlockVar &second = bbdb.bb;
            bbUserConditions.push_back(CondBBDominate(first, second));
            return second;
          }
      },
      bbp
  );
}

Errorable<void> PcodeGraph::validate() {

  Errorable<void> res;
  for (auto &[_id, gpn] : pnodes) {
    res = validatePnode(*gpn);
    if (!res.has_value()) {
      return res;
    }
  }

  for (auto &[_id, gvn] : varnodes) {
    res = validateVarnode(*gvn);
    if (!res.has_value()) {
      return res;
    }
  }
}

void PcodeGraph::updateMaxArgForPnodes(bool actionStep) {
  for (auto &[_id, gpn] : pnodes) {
    gpn->edges.updateMaxArgNum(actionStep);
  }
}

}; // namespace graph