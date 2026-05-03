#pragma once

#include "validate/action_pcodegraph.hh"
#include "validate/inferencer.hh"

class ValidateDriver {

  graph::PcodeGraph pgraph;
  std::optional<graph::ActionPcodeGraph> actgraph;

  solvers::SizeSolver sizesolver;
  solvers::BBSolver bbsolver;

  Context &cntx;
  infer::Inferencer inferencer;

  std::vector<speccond::SpecCondition> runtimeConditions;
  std::vector<speccond::SpecCondition> requiredConditions; // from action step

  graph::ActionPcodeGraph &getActgraph() { return actgraph.value(); }

public:
  ValidateDriver(Context &_context)
      : pgraph(), actgraph(std::nullopt), sizesolver(), bbsolver(),
        cntx(_context), inferencer(cntx, pgraph, sizesolver, bbsolver) {};
  Errorable<void> validate(const ast::Rule &rule);

  const std::vector<speccond::SpecCondition> &getRTC() const;
  const std::vector<speccond::SpecCondition> &getRQC() const;
};