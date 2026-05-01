#pragma once

#include "validate/pcodegraph.hh"

namespace graph {

struct NewVarGraphNode : VarGraphNode {
  std::vector<ast::Size> specSizes;

  NewVarGraphNode(const ast::Id &_id) : VarGraphNode(_id) {};
};

struct NewOpGraphNode : OpGraphNode {
  bool isInsertBefore; // True - Before; False - After
  ast::Id oldVarId;

  NewOpGraphNode(const ast::Id &_id) : OpGraphNode(_id) {}
  NewOpGraphNode(const OpGraphNode &ogn) : OpGraphNode(ogn) {}

  Errorable<void> addSpec(const ast::PnodeSpecTypeAndLoc &spec);
};

class ActionPcodeGraph : public PcodeGraph {
  friend class infer::Inferencer;

protected:
  Errorable<GraphVarnode *> disconnectFromOutVarnode(GraphPnode *gp);

  GraphVarnode *createEmptyGraphNode();

  Errorable<GraphPnode *> disconnectFromDefPnode(GraphVarnode *gvn);
  Errorable<void> deletePnode(GraphPnode *gp);

  GraphVarnode *findOrCreateVarnode(const ast::Id &id) override;
  GraphPnode *findOrCreatePnode(const ast::Id &id) override;

  Errorable<GraphVarnode *>
  addVarnodeActionTerm(const ast::VarnodeActionTerm &vt);
  Errorable<GraphPnode *> addPnodeActionTerm(const ast::PnodeActionTerm &pt);

  Errorable<GraphVarnode *> addVarnodeAction(const ast::VarnodeAction &va);
  Errorable<GraphPnode *> addPnodeAction(const ast::PnodeAction &pa);
  Errorable<void> addEmptyAction(const ast::EmptyAction &ea);
  Errorable<void> addAction(const ast::RuleAction &act);

  Errorable<void> validateNewVarnode(const NewVarGraphNode &node);
  Errorable<void> validateNewPnode(const NewOpGraphNode &node);

public:
  ActionPcodeGraph(const PcodeGraph &base) : PcodeGraph(base) {};

  Errorable<void> addActions(const std::vector<ast::RuleAction> &acts);
  Errorable<void> validate();

protected:
  std::unordered_map<ast::Id, NewVarGraphNode *> newVarnodes;

  std::unordered_map<ast::Id, NewOpGraphNode *> newPnodes;
};
}; // namespace graph
