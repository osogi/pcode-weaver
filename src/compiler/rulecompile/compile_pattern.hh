#pragma once

#include "compiled_rule.hh"
#include "common/error.hh"
#include "validate/pcodegraph.hh"
#include "validate/runtime_value_requirements.hh"
#include "validate/specconditions.hh"

#include <vector>

namespace rulecompile {

Errorable<pcodeweaver::compiled::PatternProgram> compilePattern(
    const graph::PcodeGraph &pgraph,
    const std::vector<speccond::SpecCondition> &runtimeChecks,
    const RuntimeValueRequirements &runtimeValueRequirements
);

} // namespace rulecompile
