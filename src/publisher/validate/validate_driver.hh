#pragma once

#include "validate/inferencer.hh"

class ValidateDriver {

  graph::PcodeGraph pgraph;
  solvers::SizeSolver sizesolver;
  solvers::BBSolver bbsolver;

  Context &cntx;
  infer::Inferencer inferencer;

  std::vector<speccond::SpecCondition> runtimeConditions;
  std::vector<speccond::SpecCondition> requiredConditions; // from action step

public:
  ValidateDriver(Context &_context)
      : pgraph(), sizesolver(), bbsolver(), cntx(_context),
        inferencer(cntx, pgraph, sizesolver, bbsolver) {};

  Errorable<void> validate(const ast::Rule &rule);

  const std::vector<speccond::SpecCondition> & getRTC() const;
  const std::vector<speccond::SpecCondition> & getRQC() const;
};