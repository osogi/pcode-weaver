#pragma once

#include "parse/ast.hh"
#include "parse/context.hh"

#include "validate/conditions.hh"

namespace graph {
class OpGraphNode; // forward decl
using GraphPnode = OpGraphNode;

// Varnodes

struct VarnodeEdges {
  std::optional<GraphPnode *> def; // nullopt --- unknown; nullptr --- no def
  std::vector<GraphPnode *> descend;

  VarnodeEdges() : def(std::nullopt), descend(0) {};
};

struct VarGraphNode {
  const ast::Id &id;

  ast::VarnodeType systemType;
  std::vector<ast::VarnodeType> userTypes;

  VarnodeEdges edges;
  VarGraphNode(const ast::Id &_id) : id(_id), userTypes(0), edges() {};
};

struct ConstGraphNode {
  const ast::VarnodeConst &origVarnode;

  VarnodeEdges edges;
  ConstGraphNode(const ast::VarnodeConst &vn) : origVarnode(vn), edges() {}
};

struct EmptyGraphNode {
  const ast::VarnodeEmpty &origVarnode;

  VarnodeEdges edges;
  EmptyGraphNode(const ast::VarnodeEmpty &vn) : origVarnode(vn), edges() {}
};

using GraphVarnode = std::variant<VarGraphNode, ConstGraphNode, EmptyGraphNode>;

VarnodeEdges &getEdges(GraphVarnode *gv);
std::string toStr(const GraphVarnode *gv);
bool isEmpty(const GraphVarnode *gv);

// Pnodes
struct PnodeEdges {
  std::unordered_map<size_t, GraphVarnode *>
      inrefs;           //< The ordered list of input Varnodes for this op
  GraphVarnode *output; //< The one possible output Varnode of this op

  // For InVarnodeConditionsSpecial
  size_t maxAgrNumPattern;
  size_t maxAgrNumAction;

  PnodeEdges()
      : output(nullptr), maxAgrNumPattern(0),
        maxAgrNumAction(maxAgrNumPattern) {};

  size_t updateMaxArgNum(bool onlyAction);
};

using NestedPnode = std::variant<ast::PnodeVar, ast::PnodeVarWithType>;

struct OpGraphNode {
  const ast::Id &id;
  std::optional<ast::OpType> opTp;

  ast::BasicBlockVar systemBb;
  std::vector<ast::BasicBlockVar> userBbs;

  PnodeEdges edges;

  OpGraphNode(const ast::Id &_id)
      : id(_id), opTp(std::nullopt), userBbs(0), edges() {}
};

using GraphNode = std::variant<GraphVarnode, GraphPnode>;

class PcodeGraph {
private:
  GraphVarnode *uniqAddToNodes(const GraphVarnode &gvn);
  GraphPnode *uniqAddToNodes(const OpGraphNode &pn);

  // returns the guaranteed VarGraphNode
  GraphVarnode *findOrCreateVarnode(const ast::Id &id);
  // returns the guaranteed OpGraphNode
  GraphPnode *findOrCreatePnode(const ast::Id &id);

  Errorable<void> validateVarnode(const GraphVarnode &gvn);
  Errorable<void> validatePnodeInSpecial(
      const GraphPnode &gpn, const ast::InVarnodeConditionsSpecial &spec
  );
  Errorable<void> validatePnodeInArray(
      const GraphPnode &gpn, const ast::InVarnodeConditionsArray &arr
  );
  Errorable<void> validatePnodeOut(
      const GraphPnode &gpn, const ast::OutVarnodeCondition &outCond
  );
  Errorable<void> validatePnode(const GraphPnode &gpn);

public:
  GraphVarnode *addVarnodeTerm(const ast::VarnodeTerm &vt);
  Errorable<GraphPnode *> addPnodeTerm(const ast::PnodeTerm &pt);

  Errorable<GraphVarnode *> addVarnodePattern(const ast::VarnodePattern &vp);
  Errorable<GraphPnode *> addPnodePattern(const ast::PnodePattern &pp);
  const ast::BasicBlockVar &addBBPattern(const ast::BasicBlockPattern &bbp);
  Errorable<void> validate();
  void updateMaxArgForPnodes(bool patternStep);

private:
  std::vector<std::unique_ptr<GraphNode>> nodes;

  // store guaranteed VarGraphNode
  std::unordered_map<ast::Id, GraphVarnode *> varnodes;

  // store guaranteed OpGraphNode
  std::unordered_map<ast::Id, GraphPnode *> pnodes;

  std::vector<ConditionBBDominate> bbUserConditions;

  Context &cntx;
};
}; // namespace graph