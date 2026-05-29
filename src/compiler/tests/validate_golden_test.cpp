#include "golden_common.hh"
#include "parse/driver.hh"
#include "validate/validate_driver.hh"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

void printConditionList(
    std::ostream &out, const std::string &label,
    const std::vector<speccond::SpecCondition> &conditions
) {
  out << label << ":\n";
  if (conditions.empty()) {
    out << "  <none>\n";
    return;
  }

  for (const speccond::SpecCondition &condition : conditions) {
    out << "  " << condition << "\n";
  }
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

  ValidateDriver validateDriver(parseDriver.getContext());
  const Errorable<void> validateRes =
      validateDriver.validate(parseDriver.getParsedRule());

  output << "validate_status: " << (validateRes.has_value() ? 0 : 1) << "\n";
  if (!validateRes.has_value()) {
    output << "validate_error: " << validateRes.error().message() << "\n";
    return output.str();
  }

  printConditionList(
      output, "generated_user_conditions", validateDriver.getURTC()
  );
  printConditionList(output, "required_conditions", validateDriver.getRQC());

  return output.str();
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <validate-fixture.rule>\n";
    return 2;
  }

  const fs::path fixturePath = argv[1];
  return golden::verifyFixtureOutput(fixturePath, renderFixture(fixturePath));
}
