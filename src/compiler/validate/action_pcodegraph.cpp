#include "validate/action_pcodegraph.hh"
#include "action_pcodegraph.hh"

// this should be changed and replaced by special collection for inrefs
static const size_t MAX_ARG_NEW_NODE = 256;

namespace graph {
Errorable<void> NewOpGraphNode::addSpec(const ast::PnodeSpecTypeAndLoc &spec) {
  if (this->opTp.has_value()) {
    return err("Pnode " + this->id.getName() + " already specialised");
  }

  this->opTp = spec.opType; // will not update ids inside schema
  this->oldVarId = spec.oldVar.id;
  this->isInsertBefore = spec.isInsertBefore;
  return {};
};
Errorable<GraphVarnode *>
ActionPcodeGraph::disconnectFromOutVarnode(GraphPnode *gp) {
  const OpGraphNode &og = *unpackGP(*gp);
  if (og.edges.output != nullptr) {
    graph::getEdges(og.edges.output).def = nullptr;
    return og.edges.output;
  }

  return err(
      "Pnode " + og.id.getName() +
      " should has explicit out (as varnode or EMPTY)"
  );
}

GraphVarnode *ActionPcodeGraph::createEmptyGraphNode() {
  return uniqAddToNodes<GraphVarnode>(EmptyGraphNode(ast::VarnodeEmpty{}));
}
Errorable<GraphPnode *>
ActionPcodeGraph::disconnectFromDefPnode(GraphVarnode *gvn) {
  graph::VarnodeEdges &vedges = graph::getEdges(gvn);
  if (vedges.def.has_value()) {
    GraphPnode *pdef = vedges.def.value();
    if (pdef != nullptr) {
      GraphVarnode *emptyTmp = createEmptyGraphNode();
      unpackGP(*pdef)->edges.output = emptyTmp;
      getEdges(emptyTmp).def = pdef;
    }
    return pdef;
  }

  return err(
      "Varnode " + graph::toStr(gvn) + " should has explicit define pnode"
  );
}
Errorable<void> ActionPcodeGraph::deletePnode(GraphPnode *gp) {
  OpGraphNode &og = *unpackGP(*gp);

  if (newPnodes.contains(og.id)) {
    return err("Trying to delete new created node " + og.id.getName());
  }

  auto disRes = disconnectFromOutVarnode(gp);
  if (!disRes.has_value()) {
    return std::unexpected(disRes.error());
  }
  og.edges.output = nullptr;

  for (const auto &[_argNum, gv] : og.edges.inrefs) {
    getEdges(gv).eraseFromDescend(gp);
  }
  og.edges.inrefs.clear();

  og.deleted = true;

  return {};
};
Errorable<GraphPnode *>
ActionPcodeGraph::validateExistingPnodeActionTarget(GraphPnode *gp) {
  const OpGraphNode &og = *unpackGP(*gp);
  if (newPnodes.contains(og.id)) {
    return gp;
  }

  if (!og.opTp.has_value()) {
    return err(
        "Action cannot operate on pnode " + og.id.getName() +
        " because its operation type is undefined"
    );
  }

  return gp;
}

GraphVarnode *ActionPcodeGraph::findOrCreateVarnode(const ast::Id &id) {
  auto it = varnodes.find(id);
  if (it != varnodes.end()) {
    return it->second;
  } else {
    NewVarGraphNode vgn(id);
    vgn.edges.def = nullptr;

    varnodes[id] = uniqAddToNodes<GraphVarnode>(vgn);
    NewVarGraphNode *castRes = dynamic_cast<NewVarGraphNode *>(
        get_if_uniq<VarGraphNode>(varnodes[id])
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

    pnodes[id] = uniqAddToNodes<GraphPnode>(pn);
    GraphPnode *res = pnodes[id];
    NewOpGraphNode *castRes =
        dynamic_cast<NewOpGraphNode *>(unpackGP(*res).get());
    assert(castRes != nullptr);
    newPnodes[id] = castRes;

    GraphVarnode *tmpEmpty = createEmptyGraphNode();
    VarnodeEdges &tmpEmptyEdges = getEdges(tmpEmpty);

    for (size_t i = 0; i < MAX_ARG_NEW_NODE; i++) {
      castRes->edges.inrefs[i] = tmpEmpty;
      tmpEmptyEdges.descend.push_back(res);
    }
    castRes->edges.output = tmpEmpty;
    tmpEmptyEdges.def = res;

    return res;
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
          [this](const ast::PnodeVar &x) {
            return addPnodeTerm(x).and_then(
                [this](GraphPnode *gp) -> Errorable<GraphPnode *> {
                  return validateExistingPnodeActionTarget(gp);
                }
            );
          },
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
              auto specRes = it->second->addSpec(spec);
              if (!specRes.has_value()) {
                return std::unexpected(specRes.error());
              }
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

            auto pres = disconnectFromOutVarnode(gp);
            unpackGP(*gp)->edges.output = gv;
            if (!pres.has_value()) {
              return std::unexpected(pres.error());
            }

            auto vres = disconnectFromDefPnode(gv);
            getEdges(gv).def = gp;
            if (!vres.has_value()) {
              return std::unexpected(vres.error());
            }

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

            PnodeEdges &pnodeEdges = unpackGP(*gp)->edges;
            if (pnodeEdges.inrefs.contains(argNum)) {
              getEdges(pnodeEdges.inrefs[argNum]).eraseFromDescend(gp);
            }
            pnodeEdges.inrefs[argNum] = gv;

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
    Errorable<void> res = addAction(act);
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
  for (const auto &[_id, newPnode] : newPnodes) {
    newPnode->edges.maxAgrNumPattern = newPnode->edges.maxAgrNumAction;
  }

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

bool ActionPcodeGraph::isNewVarnode(const ast::Id &id) const {
  return newVarnodes.contains(id);
}

bool ActionPcodeGraph::isNewPnode(const ast::Id &id) const {
  return newPnodes.contains(id);
}

bool ActionPcodeGraph::containsNewNodeValue(
    const specvalues::SpecValueSize &value
) const {
  return std::visit(
      util::overloaded{
          [](const specvalues::ConcreateSize &) { return false; },
          [this](const specvalues::SizeOfVarnode &value) {
            return isNewVarnode(value.nodeId);
          },
      },
      value
  );
}

bool ActionPcodeGraph::containsNewNodeValue(
    const specvalues::SpecValueBB &value
) const {
  return std::visit(
      util::overloaded{
          [this](const specvalues::BBOfPnode &value) {
            return isNewPnode(value.nodeId);
          },
          [this](const specvalues::BBOfVarnode &value) {
            return isNewVarnode(value.nodeId);
          },
      },
      value
  );
}

bool ActionPcodeGraph::containsNewNodeValue(
    const speccond::SpecCondition &condition
) const {
  return std::visit(
      util::overloaded{
          [this](const speccond::SizeEqual &condition) {
            return containsNewNodeValue(condition.a) ||
                   containsNewNodeValue(condition.b);
          },
          [this](const speccond::BBEqual &condition) {
            return containsNewNodeValue(condition.a) ||
                   containsNewNodeValue(condition.b);
          },
          [this](const speccond::BBDominate &condition) {
            return containsNewNodeValue(condition.a) ||
                   containsNewNodeValue(condition.b);
          },
      },
      condition
  );
}

void ActionPcodeGraph::removeConditionsWithNewNodes(
    std::vector<speccond::SpecCondition> &conditions
) const {
  std::erase_if(conditions, [this](const auto &condition) {
    return containsNewNodeValue(condition);
  });
}

const NewOpGraphNode *ActionPcodeGraph::getNewPnode(const ast::Id &id) const {
  auto it = newPnodes.find(id);
  return it == newPnodes.end() ? nullptr : it->second;
}

std::vector<ast::Id> ActionPcodeGraph::getNewVarnodeIds() const {
  std::vector<ast::Id> ids;
  ids.reserve(newVarnodes.size());
  for (const auto &[id, _] : newVarnodes) {
    ids.push_back(id);
  }
  return ids;
}

std::vector<ast::Id> ActionPcodeGraph::getNewPnodeIds() const {
  std::vector<ast::Id> ids;
  ids.reserve(newPnodes.size());
  for (const auto &[id, _] : newPnodes) {
    ids.push_back(id);
  }
  return ids;
}

}; // namespace graph
