// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "validate/pcodegraph.hh"
#include "pcodegraph.hh"

namespace graph {

VarnodeEdges &getEdges(GraphVarnode *gv) {
  return std::visit(
      util::overloaded{
          [](unq<VarGraphNode> &v) -> VarnodeEdges & { return v->edges; },
          [](unq<ConstGraphNode> &v) -> VarnodeEdges & { return v->edges; },
          [](unq<EmptyGraphNode> &v) -> VarnodeEdges & { return v->edges; },
      },
      *gv
  );
}

const VarnodeEdges &getEdges(const GraphVarnode *gv) {
  return std::visit(
      util::overloaded{
          [](const unq<VarGraphNode> &v) -> const VarnodeEdges & {
            return v->edges;
          },
          [](const unq<ConstGraphNode> &v) -> const VarnodeEdges & {
            return v->edges;
          },
          [](const unq<EmptyGraphNode> &v) -> const VarnodeEdges & {
            return v->edges;
          },
      },
      *gv
  );
}

std::string toStr(const GraphVarnode *gv) {
  return std::visit(
      util::overloaded{
          [](const unq<VarGraphNode> &v) -> std::string {
            return v->id.getName();
          },
          [](const unq<ConstGraphNode> &v) -> std::string {
            return "#" + std::to_string(v->origVarnode.value);
          },
          [](const unq<EmptyGraphNode> &v) -> std::string { return "EMPTY"; },
      },
      *gv
  );
}

bool isEmpty(const GraphVarnode *gv) {
  return std::holds_alternative<unq<EmptyGraphNode>>(*gv);
}

int32_t PnodeEdges::updateMaxArgNum(bool actionOnly) {
  int32_t maxArg = -1;
  for (auto &[argNum, gv] : inrefs) {
    if (!isEmpty(gv)) {
      if (maxArg < static_cast<int64_t>(argNum)) {
        maxArg = argNum;
      }
    }
  }

  if (!actionOnly) {
    maxAgrNumPattern = maxArg;
  }
  maxAgrNumAction = maxArg;

  return maxArg;
}

// since GraphPnode has only one element in the variant
const unq<OpGraphNode> &unpackGP(const GraphPnode &x) {
  static_assert(
      std::is_same_v<GraphPnode, std::variant<unq<OpGraphNode>>>,
      "Expected GraphPnode is std::variant<unq<OpGraphNode>>"
  );
  return std::get<unq<OpGraphNode>>(x);
}

bool isDeleted(const GraphVarnode &x) { return false; }

bool isDeleted(const GraphPnode &x) { return unpackGP(x)->deleted; }

bool isDeleted(const GraphNode &gn) {
  return std::visit([](const auto &x) { return isDeleted(x); }, gn);
}

bool &isGhost(GraphVarnode *gv) {
  return std::visit([](auto &v) -> bool & { return v->isGhost; }, *gv);
}

bool &isGhost(GraphPnode *gp) { return unpackGP(*gp)->isGhost; }

PcodeGraph::PcodeGraph(const PcodeGraph &other)
    : nodes(), varnodes(), pnodes(), bbUserConditions(other.bbUserConditions),
      bbEdgeUserConditions(other.bbEdgeUserConditions) {
  std::unordered_map<const GraphVarnode *, GraphVarnode *> varnodeCopies;
  std::unordered_map<const GraphPnode *, GraphPnode *> pnodeCopies;

  for (const unq<GraphNode> &oldNode : other.nodes) {
    std::visit(
        util::overloaded{
            [&](const GraphVarnode &oldVarnode) {
              GraphVarnode *newVarnode = std::visit(
                  util::overloaded{
                      [&](const unq<VarGraphNode> &node) {
                        return uniqAddToNodes<GraphVarnode>(*node);
                      },
                      [&](const unq<ConstGraphNode> &node) {
                        return uniqAddToNodes<GraphVarnode>(*node);
                      },
                      [&](const unq<EmptyGraphNode> &node) {
                        return uniqAddToNodes<GraphVarnode>(*node);
                      },
                  },
                  oldVarnode
              );
              varnodeCopies[&oldVarnode] = newVarnode;
            },
            [&](const GraphPnode &oldPnode) {
              GraphPnode *newPnode =
                  uniqAddToNodes<GraphPnode>(*unpackGP(oldPnode));
              pnodeCopies[&oldPnode] = newPnode;
            },
        },
        *oldNode
    );
  }

  for (const auto &[id, oldVarnode] : other.varnodes) {
    varnodes[id] = varnodeCopies.at(oldVarnode);
  }
  for (const auto &[id, oldPnode] : other.pnodes) {
    pnodes[id] = pnodeCopies.at(oldPnode);
  }

  auto remapVarnode = [&](GraphVarnode *oldVarnode) -> GraphVarnode * {
    return oldVarnode == nullptr ? nullptr : varnodeCopies.at(oldVarnode);
  };
  auto remapPnode = [&](GraphPnode *oldPnode) -> GraphPnode * {
    return oldPnode == nullptr ? nullptr : pnodeCopies.at(oldPnode);
  };

  for (const unq<GraphNode> &oldNode : other.nodes) {
    std::visit(
        util::overloaded{
            [&](const GraphVarnode &oldVarnode) {
              GraphVarnode *newVarnode = varnodeCopies.at(&oldVarnode);
              VarnodeEdges &newEdges = getEdges(newVarnode);
              const VarnodeEdges &oldEdges = getEdges(&oldVarnode);

              if (oldEdges.def.has_value()) {
                newEdges.def = remapPnode(oldEdges.def.value());
              } else {
                newEdges.def = std::nullopt;
              }

              newEdges.descend.clear();
              for (GraphPnode *oldDescend : oldEdges.descend) {
                newEdges.descend.push_back(remapPnode(oldDescend));
              }
            },
            [&](const GraphPnode &oldPnode) {
              GraphPnode *newPnode = pnodeCopies.at(&oldPnode);
              PnodeEdges &newEdges = unpackGP(*newPnode)->edges;
              const PnodeEdges &oldEdges = unpackGP(oldPnode)->edges;

              newEdges.output = remapVarnode(oldEdges.output);
              newEdges.inrefs.clear();
              for (const auto &[argNum, oldVarnode] : oldEdges.inrefs) {
                newEdges.inrefs[argNum] = remapVarnode(oldVarnode);
              }
            },
        },
        *oldNode
    );
  }
}

// returns the guaranteed VarGraphNode
GraphVarnode *PcodeGraph::findOrCreateVarnode(const ast::Id &id) {
  auto it = varnodes.find(id);
  if (it != varnodes.end()) {
    return it->second;
  } else {
    VarGraphNode vn(id);

    varnodes[id] = uniqAddToNodes<GraphVarnode>(vn);
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

    pnodes[id] = uniqAddToNodes<GraphPnode>(pn);
    return pnodes[id];
  }
}
Errorable<void> PcodeGraph::validateVarnode(const GraphVarnode &gvn) {
  return std::visit(
      util::overloaded{
          [](const unq<VarGraphNode> &) -> Errorable<void> { return {}; },
          [&](const unq<ConstGraphNode> &uptr) -> Errorable<void> {
            const ConstGraphNode &cnst = *uptr;

            if (cnst.edges.def.has_value() &&
                cnst.edges.def.value() != nullptr) {
              return err(
                  "Const varnode " + std::to_string(cnst.origVarnode.value) +
                  " defined by pnode " +
                  unpackGP(*cnst.edges.def.value())->id.getName()
              );
            }
            return {};
          },
          [&](const unq<EmptyGraphNode> &cnst) -> Errorable<void> {
            return {};
          },
      },
      gvn
  );
}
Errorable<void> PcodeGraph::validatePnodeInSpecial(
    const OpGraphNode &og, const ast::InVarnodeConditionsSpecial &spec
) {
  (void)spec;
  int32_t patMaxArg = og.edges.maxAgrNumPattern;
  int32_t actMaxArg = og.edges.maxAgrNumAction;

  if (patMaxArg != actMaxArg) {
    return err(
        "Action cannot change number of args for pnode " + og.id.getName() +
        " with special input conditions: pattern max arg is " +
        std::to_string(patMaxArg) + ", action max arg is " +
        std::to_string(actMaxArg)
    );
  }

  int32_t maxArgNum = patMaxArg;
  for (const auto &[argNum, gv] : og.edges.inrefs) {
    if (static_cast<int64_t>(argNum) <= maxArgNum) {
      if (isEmpty(gv)) {
        return err(
            "Pnode " + og.id.getName() + " has EMPTY as " +
            std::to_string(argNum) +
            " arg. Not expected for all args <= " + std::to_string(argNum)
        );
      }
    }
  }

  return {};
}
Errorable<void> PcodeGraph::validatePnodeInArray(
    const OpGraphNode &og, const ast::InVarnodeConditionsArray &arr
) {
  for (auto &[argNum, gv] : og.edges.inrefs) {

    if (isEmpty(gv)) {
      if (argNum < arr.array.size()) {
        return err(
            "Unexpected " + toStr(gv) + " as " + std::to_string(argNum) +
            " arg of pnode " + og.id.getName() + " expected non-Empty"
        );
      }
    } else {
      if (argNum >= arr.array.size()) {
        return err(
            "Unexpected " + toStr(gv) + " as " + std::to_string(argNum) +
            " arg of pnode " + og.id.getName() + " expected Empty"
        );
      }
    }
  }
  return {};
}
Errorable<void> PcodeGraph::validatePnodeOut(
    const OpGraphNode &og, const ast::OutVarnodeCondition &outCond
) {
  if (og.edges.output != nullptr) {
    if (isEmpty(og.edges.output)) {
      if (std::holds_alternative<ast::OutVarnodeConditionDefault>(outCond)) {
        return err("Expected see non-empty output of pnode " + og.id.getName());
      }
    } else {
      if (std::holds_alternative<ast::OutVarnodeConditionNoOut>(outCond)) {
        return err(
            "Expected see empty output of pnode " + og.id.getName() +
            ", but get " + toStr(og.edges.output)
        );
      }
    }
  }
  return {};
}
Errorable<void> PcodeGraph::validatePnode(const GraphPnode &gpn) {
  const OpGraphNode &og = *unpackGP(gpn);
  if (og.opTp.has_value()) {
    const ast::OpType &opTp = og.opTp.value();
    auto res = std::visit(
        util::overloaded{
            [&](const ast::InVarnodeConditionsArray &arr) {
              return validatePnodeInArray(og, arr);
            },
            [&](const ast::InVarnodeConditionsSpecial &spec) {
              return validatePnodeInSpecial(og, spec);
            },
        },
        opTp.scheme.inVarnodeConds
    );
    if (!res.has_value()) {
      return res;
    }

    res = validatePnodeOut(og, opTp.scheme.outVarnodeCond);
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
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
            VarGraphNode *vgn = get_if_force<unq<VarGraphNode>>(gvn)->get();
            vgn->userTypes.push_back(vn.vntype);
            return gvn;
          },
          [this](const ast::VarnodeEmpty &orig) {
            return uniqAddToNodes<GraphVarnode>(EmptyGraphNode{orig});
          },
          [this](const ast::VarnodeConst &orig) {
            return uniqAddToNodes<GraphVarnode>(ConstGraphNode{orig});
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
            OpGraphNode *pgn = unpackGP(*gpn).get();
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
          [](const ast::PnodeEmpty &) -> Errorable<GraphPnode *> {
            return nullptr;
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
                  auto &vIn = getEdges(gv).def;
                  if (gp == nullptr) {
                    if (vIn.has_value()) {
                      if (vIn.value() == nullptr) {
                        return gv;
                      }
                      return err(
                          "Varnode " + toStr(gv) +
                          " already had def pnode (" +
                          unpackGP(*vIn.value())->id.getName() +
                          ") during adding EMPTY as new one"
                      );
                    }
                    vIn = nullptr;
                    return gv;
                  }

                  const unq<OpGraphNode> &og = unpackGP(*gp);
                  auto &pOut = og->edges.output;

                  if (pOut != nullptr) {
                    return err(
                        "Pnode " + og->id.getName() + " already had output (" +
                        toStr(pOut) + ") during adding " + toStr(gv) +
                        " as new one"
                    );
                  }
                  pOut = gv;

                  if (vIn.has_value()) {
                    if (vIn.value() == nullptr) {
                      return err(
                          "Varnode " + toStr(gv) +
                          " already had EMPTY def pnode during adding " +
                          og->id.getName() + " as new one"
                      );
                    }
                    return err(
                        "Varnode " + toStr(gv) + " already had def pnode (" +
                        unpackGP(*vIn.value())->id.getName() +
                        ") during adding " + og->id.getName() + " as new one"
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
                    if (pn == nullptr) {
                      return err(
                          "EMPTY pnode cannot take " + toStr(vn) +
                          " as an argument"
                      );
                    }

                    OpGraphNode &og = *unpackGP(*pn);

                    uint32_t n = nArg.num;
                    if (og.edges.inrefs.contains(n)) {
                      return err(
                          "Pnode " + og.id.getName() + " already had " +
                          toStr(og.edges.inrefs[n]) + " as arg number " +
                          std::to_string(n) + " during adding " + toStr(vn) +
                          " as new one"
                      );
                    };

                    og.edges.inrefs[n] = vn;
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
          [this](const ast::BasicBlockVar &bbv) -> const ast::BasicBlockVar & {
            return bbv;
          },
          [this](const ast::Box<ast::BasicBlockDominatedBy> &b)
              -> const ast::BasicBlockVar & {
            const ast::BasicBlockDominatedBy &bbdb = *b;
            const ast::BasicBlockVar &first = addBBPattern(bbdb.bbp);
            const ast::BasicBlockVar &second = bbdb.bb;
            bbUserConditions.push_back(CondBBDominate(first, second));
            return second;
          },
          [this](const ast::Box<ast::BasicBlockOutgoingEdge> &b)
              -> const ast::BasicBlockVar & {
            const ast::BasicBlockOutgoingEdge &edge = *b;
            const ast::BasicBlockVar &first = addBBPattern(edge.bbp);
            const ast::BasicBlockVar &second = edge.bb;
            bbEdgeUserConditions.push_back(
                CondBBOutgoingEdge(first, second, edge.outIndex)
            );
            return second;
          }
      },
      bbp
  );
}
Errorable<void> PcodeGraph::addPattern(const ast::RulePattern &p) {
  return std::visit(
      util::overloaded{
          [&](const ast::VarnodePattern &x) -> Errorable<void> {
            auto res = addVarnodePattern(x);
            if (!res.has_value()) {
              return std::unexpected(res.error());
            } else {
              return {};
            }
          },
          [&](const ast::PnodePattern &x) -> Errorable<void> {
            auto res = addPnodePattern(x);
            if (!res.has_value()) {
              return std::unexpected(res.error());
            }
            if (res.value() == nullptr) {
              return err("EMPTY pnode pattern must define a varnode");
            }
            return {};
          },
          [&](const ast::BasicBlockPattern &x) -> Errorable<void> {
            addBBPattern(x);
            return {};
          }
      },
      p
  );
}
Errorable<void>
PcodeGraph::addPatterns(const std::vector<ast::RulePattern> &ps) {
  for (const auto &p : ps) {
    auto res = addPattern(p);
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
}
Errorable<void> PcodeGraph::validate() {
  for (const auto &gn : liveNodes()) {
    Errorable<void> res = std::visit(
        util::overloaded{
            [this](const GraphVarnode &gvn) { return validateVarnode(gvn); },
            [this](const GraphPnode &gpn) { return validatePnode(gpn); }
        },
        *gn
    );
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
}

void PcodeGraph::updateMaxArgForPnodes(bool actionOnly) {
  for (auto &[_id, gpn] : livePnodes()) {
    unpackGP(*gpn)->edges.updateMaxArgNum(actionOnly);
  }
}

GraphVarnode *
PcodeGraph::createGhostVarnode(Context &cntx, const std::string &nameHint) {
  ast::Id id = cntx.varnodeVarFactory.createId(nameHint);

  GraphVarnode *gv = findOrCreateVarnode(id);
  isGhost(gv) = true;
  return gv;
}

GraphVarnode *PcodeGraph::createGhostEmptyVarnode() {
  GraphVarnode *gv =
      uniqAddToNodes<GraphVarnode>(EmptyGraphNode{ast::VarnodeEmpty{}});
  isGhost(gv) = true;
  return gv;
}

GraphPnode *
PcodeGraph::createGhostPnode(Context &cntx, const std::string &nameHint) {
  ast::Id id = cntx.pnodeVarFactory.createId(nameHint);

  GraphPnode *gp = findOrCreatePnode(id);
  isGhost(gp) = true;
  return gp;
}

void PcodeGraph::pnodeTryAddInGhostNode(
    Context &cntx, GraphPnode *gp, size_t argNum
) {
  OpGraphNode *og = unpackGP(*gp).get();

  if (!og->edges.inrefs.contains(argNum)) {
    GraphVarnode *gv = createGhostVarnode(
        cntx,
        "_ghost_arg_n" + std::to_string(argNum) + "_of_" + og->id.getName()
    );
    og->edges.inrefs[argNum] = gv;
    getEdges(gv).descend.push_back(gp);
  }
}

void PcodeGraph::addGhostNodesOpGraphNodeInSpecial(
    Context &cntx, GraphPnode *gp, const ast::InVarnodeConditionsSpecial &spec
) {
  OpGraphNode *og = unpackGP(*gp).get();

  int32_t maxArg =
      std::min(og->edges.maxAgrNumPattern, og->edges.maxAgrNumAction);
  for (int32_t i = 0; i <= maxArg; i++) {
    pnodeTryAddInGhostNode(cntx, gp, static_cast<size_t>(i));
  }
};

void PcodeGraph::addGhostNodesOpGraphNodeInArray(
    Context &cntx, GraphPnode *gp, const ast::InVarnodeConditionsArray &arr
) {
  for (size_t i = 0; i < arr.array.size(); i++) {
    pnodeTryAddInGhostNode(cntx, gp, i);
  }
};

void PcodeGraph::addGhostNodesOpGraphNodeOut(
    Context &cntx, GraphPnode *gp, const ast::OutVarnodeCondition &outCond
) {
  OpGraphNode *og = unpackGP(*gp).get();
  if (og->edges.output != nullptr) {
    return;
  }

  GraphVarnode *gv = std::visit(
      util::overloaded{
          [&](const ast::OutVarnodeConditionDefault &) {
            return createGhostVarnode(
                cntx, "_ghost_out_of_" + og->id.getName()
            );
          },
          [&](const ast::OutVarnodeConditionNoOutOr &) {
            return createGhostVarnode(
                cntx, "_ghost_out_of_" + og->id.getName()
            );
          },
          [&](const ast::OutVarnodeConditionNoOut &) {
            return createGhostEmptyVarnode();
          },
      },
      outCond
  );

  og->edges.output = gv;
  getEdges(gv).def = gp;
}

void PcodeGraph::addGhostNodesOpGraphNode(Context &cntx, GraphPnode *gp) {
  OpGraphNode *og = unpackGP(*gp).get();
  if (og->isGhost || !og->opTp.has_value()) {
    return;
  }

  const ast::OpType &opTp = og->opTp.value();
  std::visit(
      util::overloaded{
          [&](const ast::InVarnodeConditionsArray &arr) {
            addGhostNodesOpGraphNodeInArray(cntx, gp, arr);
          },
          [&](const ast::InVarnodeConditionsSpecial &spec) {
            addGhostNodesOpGraphNodeInSpecial(cntx, gp, spec);
          },
      },
      opTp.scheme.inVarnodeConds
  );
  addGhostNodesOpGraphNodeOut(cntx, gp, opTp.scheme.outVarnodeCond);
}

void PcodeGraph::addGhostNodesVarGraphNode(Context &cntx, GraphVarnode *gv) {
  // VarGraphNode *vg = get_if_uniq<VarGraphNode>(gv);
  // if (vg == nullptr || vg->isGhost || vg->edges.def.has_value()) {
  //   return;
  // }

  // GraphPnode *gp = createGhostPnode(cntx, "_ghost_def_of_" + vg->id.getName());
  // OpGraphNode *og = unpackGP(*gp).get();
  // if (og->edges.output == nullptr) {
  //   og->edges.output = gv;
  //   vg->edges.def = gp;
  // }
}

void PcodeGraph::initGhostNodes(Context &cntx) {
  std::vector<GraphVarnode *> userVarnodes;
  std::vector<GraphPnode *> userPnodes;

  for (const auto &[_, gv] : liveVarnodes()) {
    if (!isGhost(gv)) {
      userVarnodes.push_back(gv);
    }
  }
  for (const auto &[_, gp] : livePnodes()) {
    if (!isGhost(gp)) {
      userPnodes.push_back(gp);
    }
  }

  for (GraphVarnode *gv : userVarnodes) {
    addGhostNodesVarGraphNode(cntx, gv);
  }
  for (GraphPnode *gp : userPnodes) {
    addGhostNodesOpGraphNode(cntx, gp);
  }
}

}; // namespace graph
