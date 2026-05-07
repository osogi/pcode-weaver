// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include "parse/ast.hh"
#include "parse/context.hh"

#include <list>
#include <ranges>
#include <unordered_map>

namespace infer {
class Inferencer; // forward decl
}

namespace graph {
template <class T> using unq = std::unique_ptr<T>;

class OpGraphNode; // forward decl
using GraphPnode = std::variant<unq<OpGraphNode>>;

// since GraphPnode has only one element in the variant
const unq<OpGraphNode> &unpackGP(const GraphPnode &x);

// Varnodes

struct VarnodeEdges {
  std::optional<GraphPnode *> def; // nullopt --- unknown; nullptr --- no def
  std::list<GraphPnode *> descend;

  VarnodeEdges() : def(std::nullopt), descend() {};

  void eraseFromDescend(GraphPnode *gp) {
    auto it = std::find(descend.begin(), descend.end(), gp);
    assert(it != descend.end());
    descend.erase(it);
  };
};

struct VarGraphNode {
  const ast::Id id;
  std::vector<ast::VarnodeType> userTypes;
  VarnodeEdges edges;
  bool isGhost = false;

  VarGraphNode(const ast::Id &_id) : id(_id), userTypes(0), edges() {};
  virtual ~VarGraphNode() {}
};

struct ConstGraphNode {
  const ast::VarnodeConst origVarnode;
  VarnodeEdges edges;
  bool isGhost = false;

  ConstGraphNode(const ast::VarnodeConst &vn) : origVarnode(vn), edges() {}
};

struct EmptyGraphNode {
  const ast::VarnodeEmpty origVarnode;
  VarnodeEdges edges;
  bool isGhost = false;

  EmptyGraphNode(const ast::VarnodeEmpty &vn) : origVarnode(vn), edges() {}
};

using GraphVarnode =
    std::variant<unq<VarGraphNode>, unq<ConstGraphNode>, unq<EmptyGraphNode>>;

VarnodeEdges &getEdges(GraphVarnode *gv);
const VarnodeEdges &getEdges(const GraphVarnode *gv);
std::string toStr(const GraphVarnode *gv);
bool isEmpty(const GraphVarnode *gv);
bool isDeleted(const GraphVarnode &x);
bool &isGhost(GraphVarnode *gv);

// Pnodes
struct PnodeEdges {
  std::unordered_map<size_t, GraphVarnode *>
      inrefs;           //< The ordered list of input Varnodes for this op
  GraphVarnode *output; //< The one possible output Varnode of this op

  // For InVarnodeConditionsSpecial
  int32_t maxAgrNumPattern;
  int32_t maxAgrNumAction;

  PnodeEdges()
      : output(nullptr), maxAgrNumPattern(0),
        maxAgrNumAction(maxAgrNumPattern) {};

  int32_t updateMaxArgNum(bool actionOnly);
};

struct OpGraphNode {
  const ast::Id id;
  std::optional<ast::OpType> opTp;

  std::vector<ast::BasicBlockVar> userBbs;

  PnodeEdges edges;
  bool deleted;
  bool isGhost = false;

  OpGraphNode(const ast::Id &_id)
      : id(_id), opTp(std::nullopt), userBbs(0), edges(), deleted(false) {}
  virtual ~OpGraphNode() {}
};
bool isDeleted(const GraphPnode &x);
bool &isGhost(GraphPnode *x);

using GraphNode = std::variant<GraphVarnode, GraphPnode>;

bool isDeleted(const GraphNode &gn);

// a <= b
struct CondBBDominate {
  ast::BasicBlockVar a;
  ast::BasicBlockVar b;

  CondBBDominate(
      const ast::BasicBlockVar &first, const ast::BasicBlockVar &second
  )
      : a(first), b(second) {};

  std::string toString() const {
    return a.id.getName() + " <= " + b.id.getName();
  }
};

struct CondBBOutgoingEdge {
  ast::BasicBlockVar a;
  ast::BasicBlockVar b;
  std::uint32_t outIndex;

  CondBBOutgoingEdge(
      const ast::BasicBlockVar &first, const ast::BasicBlockVar &second,
      std::uint32_t index
  )
      : a(first), b(second), outIndex(index) {};

  std::string toString() const {
    return a.id.getName() + " ->(" + std::to_string(outIndex) + ") " +
           b.id.getName();
  }
};

class PcodeGraph {
  friend class infer::Inferencer;

protected:
  PcodeGraph(const PcodeGraph &other);
  PcodeGraph(PcodeGraph &&other) noexcept = default;
  PcodeGraph &operator=(PcodeGraph &&other) noexcept = default;

  template <class RetPtrType, class NodeType>
  RetPtrType *uniqAddToNodes(NodeType x) {
    GraphNode gn = std::make_unique<NodeType>(x);
    unq<GraphNode> node_ptr = std::make_unique<GraphNode>(std::move(gn));

    nodes.push_back(std::move(node_ptr));

    return get_if_force<RetPtrType>(nodes.back().get());
  }

  // returns the guaranteed VarGraphNode
  virtual GraphVarnode *findOrCreateVarnode(const ast::Id &id);
  // returns the guaranteed OpGraphNode
  virtual GraphPnode *findOrCreatePnode(const ast::Id &id);
  Errorable<void> validateVarnode(const GraphVarnode &gvn);
  Errorable<void> validatePnodeInSpecial(
      const OpGraphNode &og, const ast::InVarnodeConditionsSpecial &spec
  );
  Errorable<void> validatePnodeInArray(
      const OpGraphNode &og, const ast::InVarnodeConditionsArray &arr
  );
  Errorable<void> validatePnodeOut(
      const OpGraphNode &og, const ast::OutVarnodeCondition &outCond
  );
  Errorable<void> validatePnode(const GraphPnode &gpn);

  GraphVarnode *addVarnodeTerm(const ast::VarnodeTerm &vt);
  Errorable<GraphPnode *> addPnodeTerm(const ast::PnodeTerm &pt);
  Errorable<GraphVarnode *> addVarnodePattern(const ast::VarnodePattern &vp);
  Errorable<GraphPnode *> addPnodePattern(const ast::PnodePattern &pp);
  const ast::BasicBlockVar &addBBPattern(const ast::BasicBlockPattern &bbp);
  Errorable<void> addPattern(const ast::RulePattern &p);

  GraphVarnode *createGhostVarnode(Context &cntx, const std::string &nameHint);
  GraphVarnode *createGhostEmptyVarnode();
  GraphPnode *createGhostPnode(Context &cntx, const std::string &nameHint);

  void pnodeTryAddInGhostNode(Context &cntx, GraphPnode *gp, size_t argNum);
  void addGhostNodesOpGraphNodeInSpecial(
      Context &cntx, GraphPnode *gp, const ast::InVarnodeConditionsSpecial &spec
  );
  void addGhostNodesOpGraphNodeInArray(
      Context &cntx, GraphPnode *gp, const ast::InVarnodeConditionsArray &arr
  );
  void addGhostNodesOpGraphNodeOut(
      Context &cntx, GraphPnode *gp, const ast::OutVarnodeCondition &outCond
  );
  void addGhostNodesOpGraphNode(Context &cntx, GraphPnode *gp);
  void addGhostNodesVarGraphNode(Context &cntx, GraphVarnode *gv);

public:
  PcodeGraph() = default;
  PcodeGraph &operator=(const PcodeGraph &other) = delete;

  PcodeGraph(const std::vector<ast::RulePattern> &pats) : PcodeGraph() {
    auto res = addPatterns(pats);
    assert(res.has_value());
  };
  Errorable<void> addPatterns(const std::vector<ast::RulePattern> &ps);
  void initGhostNodes(Context &cntx);
  Errorable<void> validate();
  void updateMaxArgForPnodes(bool actionOnly);

protected:
  std::list<unq<GraphNode>> nodes;

  // store guaranteed VarGraphNode
  std::unordered_map<ast::Id, GraphVarnode *> varnodes;

  // store guaranteed OpGraphNode
  std::unordered_map<ast::Id, GraphPnode *> pnodes;

  std::vector<CondBBDominate> bbUserConditions;
  std::vector<CondBBOutgoingEdge> bbEdgeUserConditions;

public:
  auto liveNodes() const {
    return nodes | std::views::filter([](const unq<GraphNode> &p) {
             return !isDeleted(*p);
           });
    ;
  }

  auto liveVarnodes() const {
    return varnodes | std::views::filter([](const auto &kv) {
             return kv.second && !isDeleted(*kv.second);
           });
  }

  auto livePnodes() const {
    return pnodes | std::views::filter([](const auto &kv) {
             return kv.second && !isDeleted(*kv.second);
           });
  }
};
}; // namespace graph
