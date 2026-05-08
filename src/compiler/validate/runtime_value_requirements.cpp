// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "validate/runtime_value_requirements.hh"

#include "common/util.hh"
#include "validate/action_pcodegraph.hh"
#include "validate/solvers.hh"

#include <optional>

namespace {

void collectPatternGhostNodeIds(
    const graph::PcodeGraph &patternGraph,
    std::unordered_set<ast::Id> &varnodes, std::unordered_set<ast::Id> &pnodes
) {
  for (const auto &gn : patternGraph.liveNodes()) {
    std::visit(
        util::overloaded{
            [&](const graph::GraphVarnode &gv) {
              const graph::VarGraphNode *vn =
                  get_if_uniq<graph::VarGraphNode>(&gv);
              if (vn != nullptr && vn->isGhost) {
                varnodes.insert(vn->id);
              }
            },
            [&](const graph::GraphPnode &gp) {
              const graph::OpGraphNode &pn = *graph::unpackGP(gp);
              if (pn.isGhost) {
                pnodes.insert(pn.id);
              }
            },
        },
        *gn
    );
  }
}

void requireFinalSizeIfGhost(
    RuntimeValueRequirements &requirements, const solvers::SizeSolver &solver,
    const solvers::SizeTerm &term,
    const std::unordered_set<ast::Id> &ghostVarnodes
) {
  std::optional<solvers::SizeTerm> final = solver.findoptConst(term);
  if (!final.has_value()) {
    return;
  }

  const solvers::SizeValue *value = std::get_if<solvers::SizeValue>(&*final);
  if (value == nullptr) {
    return;
  }

  const specvalues::SizeOfVarnode *size =
      std::get_if<specvalues::SizeOfVarnode>(value);
  if (size != nullptr && ghostVarnodes.contains(size->nodeId)) {
    requirements.requireVarnodeSize(size->nodeId);
  }
}

void requireFinalBBIfGhost(
    RuntimeValueRequirements &requirements, const solvers::BBSolver &solver,
    const solvers::BBTerm &term,
    const std::unordered_set<ast::Id> &ghostVarnodes,
    const std::unordered_set<ast::Id> &ghostPnodes
) {
  std::optional<solvers::BBTerm> final = solver.findoptConst(term);
  if (!final.has_value()) {
    return;
  }

  const solvers::BBValue *value = std::get_if<solvers::BBValue>(&*final);
  if (value == nullptr) {
    return;
  }

  std::visit(
      util::overloaded{
          [&](const specvalues::BBOfVarnode &bb) {
            if (ghostVarnodes.contains(bb.nodeId)) {
              requirements.requireVarnodeBB(bb.nodeId);
            }
          },
          [&](const specvalues::BBOfPnode &bb) {
            if (ghostPnodes.contains(bb.nodeId)) {
              requirements.requirePnodeBB(bb.nodeId);
            }
          },
      },
      *value
  );
}

} // namespace

RuntimeValueRequirements RuntimeValueRequirements::fromActionGraph(
    const graph::PcodeGraph &patternGraph,
    const graph::ActionPcodeGraph &actionGraph,
    const solvers::SizeSolver &sizeSolver, const solvers::BBSolver &bbSolver
) {
  RuntimeValueRequirements result;
  std::unordered_set<ast::Id> ghostVarnodes;
  std::unordered_set<ast::Id> ghostPnodes;
  collectPatternGhostNodeIds(patternGraph, ghostVarnodes, ghostPnodes);

  for (const ast::Id &newVarnode : actionGraph.getNewVarnodeIds()) {
    requireFinalSizeIfGhost(
        result, sizeSolver, specvalues::SizeOfVarnode(newVarnode), ghostVarnodes
    );
    requireFinalBBIfGhost(
        result,
        bbSolver,
        specvalues::BBOfVarnode(newVarnode),
        ghostVarnodes,
        ghostPnodes
    );
  }

  for (const ast::Id &newPnode : actionGraph.getNewPnodeIds()) {
    requireFinalBBIfGhost(
        result,
        bbSolver,
        specvalues::BBOfPnode(newPnode),
        ghostVarnodes,
        ghostPnodes
    );
  }

  return result;
}
