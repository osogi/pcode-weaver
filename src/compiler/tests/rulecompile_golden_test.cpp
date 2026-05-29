#include "compiled_rule.hh"
#include "golden_common.hh"
#include "parse/driver.hh"
#include "rulecompile/rulecompile_driver.hh"
#include "validate/validate_driver.hh"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;
namespace compiled = pcodeweaver::compiled;

void printCompiledRule(std::ostream &out, const compiled::Rule &rule) {
  out << "pattern_steps_count: " << rule.pattern.steps.size() << "\n";
  out << "action_steps_count: " << rule.action.steps.size() << "\n";
}

std::string renderFixture(const fs::path &fixturePath) {
  std::ostringstream errors;
  yy::Driver parseDriver;

  int parseStatus = 0;
  {
    golden::ScopedCerrRedirect redirect(errors);
    parseStatus = parseDriver.parse(fixturePath);
  }

  std::ostringstream output;
  output << "fixture: " << golden::displayPath(fixturePath).string() << "\n";
  output << "parse_status: " << parseStatus << "\n";
  output << "stderr:\n";

  const std::string errorText = errors.str();
  output << (errorText.empty() ? "<empty>\n" : errorText);

  if (parseStatus != 0) {
    return output.str();
  }

  const ast::Rule &rule = parseDriver.getParsedRule();
  ValidateDriver validateDriver(parseDriver.getContext());
  auto validateRes = validateDriver.validate(rule);

  output << "validate_status: " << (validateRes.has_value() ? 0 : 1) << "\n";
  if (!validateRes.has_value()) {
    output << "validate_error: " << validateRes.error().message() << "\n";
    return output.str();
  }

  std::vector<speccond::SpecCondition> runtimeChecks =
      validateDriver.getRuntimeCheckConditions();
  RuleCompileDriver compileDriver(
      RuleCompileInput{
          .actions = rule.actions,
          .patternGraph = validateDriver.getPGraph(),
          .actionGraph = validateDriver.getActGraph(),
          .sizeSolver = validateDriver.getSizeSolver(),
          .runtimeChecks = runtimeChecks,
          .runtimeValueRequirements =
              validateDriver.getRuntimeValueRequirements(),
      }
  );

  auto compileRes = compileDriver.compile();
  output << "compile_status: " << (compileRes.has_value() ? 0 : 1) << "\n";
  if (!compileRes.has_value()) {
    output << "compile_error: " << compileRes.error().message() << "\n";
    return output.str();
  }

  printCompiledRule(output, compileRes.value());
  return output.str();
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <compile-fixture.rule>\n";
    return 2;
  }

  const fs::path fixturePath = argv[1];
  return golden::verifyFixtureOutput(fixturePath, renderFixture(fixturePath));
}
