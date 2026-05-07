// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include "validate/action_pcodegraph.hh"
#include "validate/inferencer.hh"
#include "validate/runtime_value_requirements.hh"

class ValidateDriver {
  graph::PcodeGraph pgraph;
  std::optional<graph::ActionPcodeGraph> actgraph;

  solvers::SizeSolver sizesolver;
  solvers::BBSolver bbsolver;

  Context &cntx;
  infer::Inferencer inferencer;

  std::vector<speccond::SpecCondition> userRuntimeConditions;
  std::vector<speccond::SpecCondition> requiredConditions; // from action step
  RuntimeValueRequirements runtimeValueRequirements;

  graph::ActionPcodeGraph &getActgraph() { return actgraph.value(); }

public:
  ValidateDriver(Context &_context)
      : pgraph(), actgraph(std::nullopt), sizesolver(), bbsolver(),
        cntx(_context), inferencer(cntx, pgraph, sizesolver, bbsolver) {};
  Errorable<void> validate(const ast::Rule &rule);

  const std::vector<speccond::SpecCondition> &getURTC() const;
  const std::vector<speccond::SpecCondition> &getRQC() const;
  std::vector<speccond::SpecCondition> getRuntimeCheckConditions() const;
  const RuntimeValueRequirements &getRuntimeValueRequirements() const;
  const graph::PcodeGraph &getPGraph() const;
  const graph::ActionPcodeGraph &getActGraph() const;
  const solvers::SizeSolver &getSizeSolver() const;
};
