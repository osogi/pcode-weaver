// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include "common/error.hh"
#include "compiled_rule.hh"
#include "validate/pcodegraph.hh"
#include "validate/runtime_value_requirements.hh"
#include "validate/specconditions.hh"

#include <unordered_map>
#include <vector>

namespace rulecompile {

struct PatternCompileResult {
  pcodeweaver::compiled::PatternProgram program;
  std::unordered_map<ast::Id, pcodeweaver::compiled::StepId> varnodeSteps;
  std::unordered_map<ast::Id, pcodeweaver::compiled::StepId> pnodeSteps;
};

Errorable<PatternCompileResult> compilePatternWithIds(
    const graph::PcodeGraph &pgraph,
    const std::vector<speccond::SpecCondition> &runtimeChecks,
    const RuntimeValueRequirements &runtimeValueRequirements
);

} // namespace rulecompile
