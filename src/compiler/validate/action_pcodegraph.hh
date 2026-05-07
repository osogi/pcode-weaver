// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include "validate/pcodegraph.hh"
#include "validate/specconditions.hh"

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
  Errorable<GraphPnode *> validateExistingPnodeActionTarget(GraphPnode *gp);

  GraphVarnode *findOrCreateVarnode(const ast::Id &id) override;
  GraphPnode *findOrCreatePnode(const ast::Id &id) override;
  Errorable<GraphVarnode *>
  addVarnodeActionTerm(const ast::VarnodeActionTerm &vt);
  Errorable<GraphPnode *> addPnodeActionTerm(const ast::PnodeActionTerm &pt);
  Errorable<GraphVarnode *> addVarnodeAction(const ast::VarnodeAction &va);
  Errorable<GraphPnode *> addPnodeAction(const ast::PnodeAction &pa);
  Errorable<void> addEmptyAction(const ast::EmptyAction &ea);
  Errorable<void> addAction(const ast::RuleAction &act);
  Errorable<void> validateExistingVarnodeDefStability(const VarGraphNode &node);
  Errorable<void> validateNewVarnode(const NewVarGraphNode &node);
  Errorable<void> validateNewPnode(const NewOpGraphNode &node);

public:
  ActionPcodeGraph(const PcodeGraph &base);
  Errorable<void> addActions(const std::vector<ast::RuleAction> &acts);
  Errorable<void> validate();
  bool isNewVarnode(const ast::Id &id) const;
  bool isNewPnode(const ast::Id &id) const;
  bool containsNewNodeValue(const specvalues::SpecValueSize &value) const;
  bool containsNewNodeValue(const specvalues::SpecValueBB &value) const;
  bool containsNewNodeValue(const speccond::SpecCondition &condition) const;
  void removeConditionsWithNewNodes(
      std::vector<speccond::SpecCondition> &conditions
  ) const;
  const NewOpGraphNode *getNewPnode(const ast::Id &id) const;
  std::vector<ast::Id> getNewVarnodeIds() const;
  std::vector<ast::Id> getNewPnodeIds() const;

protected:
  std::unordered_map<ast::Id, bool> patternVarnodeDefEmpty;

  std::unordered_map<ast::Id, NewVarGraphNode *> newVarnodes;

  std::unordered_map<ast::Id, NewOpGraphNode *> newPnodes;
};
}; // namespace graph
