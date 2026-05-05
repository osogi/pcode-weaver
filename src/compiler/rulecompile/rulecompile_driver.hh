#pragma once

#include "common/error.hh"
#include "compiled_rule.hh"
#include "validate/action_pcodegraph.hh"
#include "validate/pcodegraph.hh"
#include "validate/runtime_value_requirements.hh"
#include "validate/solvers.hh"
#include "validate/specconditions.hh"

#include <vector>

struct RuleCompileInput {
  const std::vector<ast::RuleAction> &actions;
  const graph::PcodeGraph &patternGraph;
  const graph::ActionPcodeGraph &actionGraph;
  const solvers::SizeSolver &sizeSolver;
  const std::vector<speccond::SpecCondition> &runtimeChecks;
  const RuntimeValueRequirements &runtimeValueRequirements;
};

class RuleCompileDriver {
  RuleCompileInput input;

public:
  RuleCompileDriver(const RuleCompileInput &_input) : input(_input) {}

  Errorable<pcodeweaver::compiled::Rule> compile() const;
};
