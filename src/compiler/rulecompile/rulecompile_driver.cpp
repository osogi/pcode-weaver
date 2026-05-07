// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "rulecompile/rulecompile_driver.hh"

#include "rulecompile/compile_action.hh"
#include "rulecompile/compile_pattern.hh"

Errorable<pcodeweaver::compiled::Rule> RuleCompileDriver::compile() const {
  auto pattern = rulecompile::compilePatternWithIds(
      input.patternGraph, input.runtimeChecks, input.runtimeValueRequirements
  );
  if (!pattern.has_value()) {
    return std::unexpected(pattern.error());
  }

  auto action = rulecompile::compileAction(
      input.actions, input.actionGraph, input.sizeSolver, pattern.value()
  );
  if (!action.has_value()) {
    return std::unexpected(action.error());
  }

  return pcodeweaver::compiled::Rule{
      .formatVersion = 1,
      .pattern = std::move(pattern->program),
      .action = std::move(action.value()),
  };
}
