#include "validate/action_pcodegraph.hh"
#include "action_pcodegraph.hh"

namespace graph {

Errorable<void> NewOpGraphNode::addSpec(const ast::PnodeSpecTypeAndLoc &spec) {
  if (this->opTp.has_value()) {
    return err("Pnode " + this->id.getName() + " already specialised");
  }

  this->opTp = spec.opType;
  this->oldVarId = spec.oldVar.id;
  this->isInsertBefore = spec.isInsertBefore;
  return {};
};

Errorable<GraphVarnode *>
ActionPcodeGraph::disconnectFromOutVarnode(GraphPnode *gp) {
  // Maybe better to change all same checks to creating "ghost" nodes
  //  and just work after with it as with common nodes.
  // But it can create some side effects and need to think about it.

  // Don't need to update if there isn't one
  if (gp->opTp.has_value() &&
      std::holds_alternative<ast::OutVarnodeConditionNoOut>(
          gp->opTp.value().scheme.outVarnodeCond
      )) {
    return nullptr;
  }
  if (gp->edges.output != nullptr) {
    graph::getEdges(gp->edges.output).def = nullptr;
    return gp->edges.output;
  }

  return err(
      "Pnode " + gp->id.getName() +
      " should has explicit out (as varnode or EMPTY)"
  );
}

GraphVarnode *ActionPcodeGraph::createEmptyGraphNode() {
  return uniqAddToNodes(EmptyGraphNode(ast::VarnodeEmpty{}));
}

Errorable<GraphPnode *>
ActionPcodeGraph::disconnectFromDefPnode(GraphVarnode *gvn) {
  graph::VarnodeEdges &vedges = graph::getEdges(gvn);
  if (vedges.def.has_value()) {
    GraphPnode *pdef = vedges.def.value();
    if (pdef != nullptr) {
      pdef->edges.output = createEmptyGraphNode();
    }
    return pdef;
  }

  return err(
      "Varnode " + graph::toStr(gvn) + " should has explicit define pnode"
  );
}

Errorable<void> ActionPcodeGraph::deletePnode(GraphPnode *gp) {
  if (newPnodes.contains(gp->id)) {
    return err("Trying to delete new created node " + gp->id.getName());
  }

  auto disRes = disconnectFromOutVarnode(gp);
  if (!disRes.has_value()) {
    return std::unexpected(disRes.error());
  }

  for (const auto &[_argNum, gv] : gp->edges.inrefs) {
    getEdges(gv).eraseFromDescend(gp);
  }

  gp->deleted = true;

  return {};
};

GraphVarnode *ActionPcodeGraph::findOrCreateVarnode(const ast::Id &id) {
  auto it = varnodes.find(id);
  if (it != varnodes.end()) {
    return it->second;
  } else {
    NewVarGraphNode vgn(id);

    varnodes[id] = uniqAddToNodes(vgn);
    NewVarGraphNode *castRes = dynamic_cast<NewVarGraphNode *>(
        get_if_force<VarGraphNode>(varnodes[id])
    );
    assert(castRes != nullptr);

    newVarnodes[id] = castRes;
    return varnodes[id];
  }
};

GraphPnode *ActionPcodeGraph::findOrCreatePnode(const ast::Id &id) {
  auto it = pnodes.find(id);
  if (it != pnodes.end()) {
    return it->second;
  } else {
    NewOpGraphNode pn(id);

    pnodes[id] = uniqAddToNodes(pn);
    NewOpGraphNode *castRes = dynamic_cast<NewOpGraphNode *>(pnodes[id]);
    assert(castRes != nullptr);

    newPnodes[id] = castRes;
    return pnodes[id];
  }
}

Errorable<GraphVarnode *>
ActionPcodeGraph::addVarnodeActionTerm(const ast::VarnodeActionTerm &vt) {
  return std::visit(
      util::overloaded{
          [this](const ast::VarnodeVar &x) -> Errorable<GraphVarnode *> {
            return addVarnodeTerm(x);
          },
          [this](const ast::VarnodeSpecSize &n) -> Errorable<GraphVarnode *> {
            GraphVarnode *gvn = addVarnodeTerm(n.newVar);
            auto it = newVarnodes.find(n.newVar.id);
            if (it == newVarnodes.end()) {
              return err(
                  "Size specialisation in action expected only for new "
                  "varnodes "
                  "not" +
                  n.newVar.id.getName()
              );
            } else {
              it->second->specSizes.push_back(n.size);
            }
            return gvn;
          },
          [this](const ast::VarnodeEmpty &x) -> Errorable<GraphVarnode *> {
            return addVarnodeTerm(x);
          },
          [this](const ast::VarnodeConst &x) -> Errorable<GraphVarnode *> {
            return addVarnodeTerm(x);
          }
      },
      vt
  );
};
Errorable<GraphPnode *>
ActionPcodeGraph::addPnodeActionTerm(const ast::PnodeActionTerm &pt) {
  return std::visit(
      util::overloaded{
          [this](const ast::PnodeVar &x) { return addPnodeTerm(x); },
          [this](const ast::PnodeSpecTypeAndLoc &spec)
              -> Errorable<GraphPnode *> {
            auto res = addPnodeTerm(spec.newVar);
            if (!res.has_value()) {
              return res;
            }

            auto it = newPnodes.find(spec.newVar.id);
            if (it == newPnodes.end()) {
              return err(
                  "Op specialisation in action expected only for new pnodes "
                  "not" +
                  spec.newVar.id.getName()
              );
            } else {
              it->second->addSpec(spec);
            }
            return res;
          }
      },
      pt
  );
};

Errorable<GraphVarnode *>
ActionPcodeGraph::addVarnodeAction(const ast::VarnodeAction &va) {
  return std::visit(
      util::overloaded{
          [this](const ast::VarnodeActionTerm &x) {
            return addVarnodeActionTerm(x);
          },
          // op ->> v
          [this](const ast::Box<ast::VarnodeSetAsPnodeOut> &b)
              -> Errorable<GraphVarnode *> {
            const ast::VarnodeSetAsPnodeOut &setAsOut = *b;

            auto respa = addPnodeAction(setAsOut.pa);
            if (!respa.has_value()) {
              return std::unexpected(respa.error());
            }
            GraphPnode *gp = respa.value();

            auto resvat = addVarnodeActionTerm(setAsOut.v);
            if (!resvat.has_value()) {
              return std::unexpected(resvat.error());
            }
            GraphVarnode *gv = resvat.value();

            disconnectFromOutVarnode(gp);
            gp->edges.output = gv;

            disconnectFromDefPnode(gv);
            getEdges(gv).def = gp;

            return gv;
          }
      },
      va
  );
};
Errorable<GraphPnode *>
ActionPcodeGraph::addPnodeAction(const ast::PnodeAction &pa) {
  return std::visit(
      util::overloaded{
          [this](const ast::PnodeActionTerm &x) -> Errorable<GraphPnode *> {
            return addPnodeActionTerm(x);
          },
          // v ->> (N) op
          [this](const ast::Box<ast::PnodeSetNthArg> &b)
              -> Errorable<GraphPnode *> {
            const ast::PnodeSetNthArg &setNthArg = *b;

            auto resva = addVarnodeAction(setNthArg.va);
            if (!resva.has_value()) {
              return std::unexpected(resva.error());
            }
            GraphVarnode *gv = resva.value();

            auto respat = addPnodeActionTerm(setNthArg.p);
            if (!respat.has_value()) {
              return std::unexpected(respat.error());
            }
            GraphPnode *gp = respat.value();
            uint32_t argNum = setNthArg.num;

            if (gp->edges.inrefs.contains(argNum)) {
              getEdges(gp->edges.inrefs[argNum]).eraseFromDescend(gp);
            }
            gp->edges.inrefs[argNum] = gv;

            getEdges(gv).descend.push_back(gp);

            return gp;
          }
      },
      pa
  );
};
Errorable<void> ActionPcodeGraph::addEmptyAction(const ast::EmptyAction &ea) {
  return std::visit(
      util::overloaded{
          [this](const ast::EmptyActionDeletePnode &dp) -> Errorable<void> {
            auto it = pnodes.find(dp.targetPnode.id);
            if (it == pnodes.end()) {
              return err("Pnode " + dp.targetPnode.id.getName() + " not found");
            }
            if (isDeleted(*it->second)) {
              return err(
                  "Pnode " + dp.targetPnode.id.getName() +
                  " already marked for del"
              );
            }

            return deletePnode(it->second);
          }
      },
      ea
  );
};

Errorable<void> ActionPcodeGraph::addAction(const ast::RuleAction &act) {
  return std::visit(
      util::overloaded{
          [this](const ast::VarnodeAction &x) {
            return addVarnodeAction(x).and_then([](auto) {
              return Errorable<void>{};
            });
            ;
          },
          [this](const ast::PnodeAction &x) {
            return addPnodeAction(x).and_then([](auto) {
              return Errorable<void>{};
            });
          },
          [this](const ast::EmptyAction &x) { return addEmptyAction(x); },
      },
      act
  );
};

Errorable<void>
ActionPcodeGraph::addActions(const std::vector<ast::RuleAction> &acts) {
  for (const ast::RuleAction &act : acts) {
    auto res = addAction(act);
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
};

Errorable<void>
ActionPcodeGraph::validateNewVarnode(const NewVarGraphNode &node) {
  return {};
}
Errorable<void> ActionPcodeGraph::validateNewPnode(const NewOpGraphNode &node) {
  if (!node.opTp.has_value()) {
    return err(
        "New pnode " + node.id.getName() + " isn't specialized (OpType not set)"
    );
  }
  return {};
};

Errorable<void> ActionPcodeGraph::validate() {
  auto res = PcodeGraph::validate();
  if (!res.has_value()) {
    return res;
  }
  for (const auto &[_id, newVarnode] : newVarnodes) {
    auto res = validateNewVarnode(*newVarnode);
    if (!res.has_value()) {
      return res;
    }
  }

  for (const auto &[_id, newPnode] : newPnodes) {
    auto res = validateNewPnode(*newPnode);
    if (!res.has_value()) {
      return res;
    }
  }
  return {};
}

}; // namespace graph