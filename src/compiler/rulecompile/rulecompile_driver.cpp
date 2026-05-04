#include "rulecompile/rulecompile_driver.hh"

#include "rulecompile/compile_pattern.hh"

Errorable<pcodeweaver::compiled::Rule> RuleCompileDriver::compile() const {
  auto pattern = rulecompile::compilePattern(
      input.patternGraph, input.runtimeChecks, input.runtimeValueRequirements
  );
  if (!pattern.has_value()) {
    return std::unexpected(pattern.error());
  }

  return pcodeweaver::compiled::Rule{
      .formatVersion = 1,
      .pattern = std::move(pattern.value()),
  };
}
