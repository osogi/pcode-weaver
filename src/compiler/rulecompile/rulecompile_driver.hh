#pragma once

#include "compiled_rule.hh"
#include "common/error.hh"
#include "validate/pcodegraph.hh"
#include "validate/runtime_value_requirements.hh"
#include "validate/specconditions.hh"

#include <vector>

struct RuleCompileInput {
  const graph::PcodeGraph &patternGraph;
  const std::vector<speccond::SpecCondition> &runtimeChecks;
  const RuntimeValueRequirements &runtimeValueRequirements;
};

class RuleCompileDriver {
  RuleCompileInput input;

public:
  RuleCompileDriver(const RuleCompileInput &_input) : input(_input) {}

  Errorable<pcodeweaver::compiled::Rule> compile() const;
};
