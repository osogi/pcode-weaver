#pragma once

#include "parse/ast.hh"

#include <unordered_set>

namespace graph {
class PcodeGraph;
class ActionPcodeGraph;
} // namespace graph

namespace solvers {
class SizeSolver;
class BBSolver;
} // namespace solvers

class RuntimeValueRequirements {
public:
  static RuntimeValueRequirements fromActionGraph(
      const graph::PcodeGraph &patternGraph,
      const graph::ActionPcodeGraph &actionGraph,
      const solvers::SizeSolver &sizeSolver,
      const solvers::BBSolver &bbSolver
  );

  void requireVarnodeSize(const ast::Id &id) { varnodeSizeIds.insert(id); }
  void requireVarnodeBB(const ast::Id &id) { varnodeBBIds.insert(id); }
  void requirePnodeBB(const ast::Id &id) { pnodeBBIds.insert(id); }

  bool needsVarnodeSize(const ast::Id &id) const {
    return varnodeSizeIds.contains(id);
  }
  bool needsVarnodeBB(const ast::Id &id) const {
    return varnodeBBIds.contains(id);
  }
  bool needsPnodeBB(const ast::Id &id) const {
    return pnodeBBIds.contains(id);
  }

private:
  std::unordered_set<ast::Id> varnodeSizeIds;
  std::unordered_set<ast::Id> varnodeBBIds;
  std::unordered_set<ast::Id> pnodeBBIds;
};
