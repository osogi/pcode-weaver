#include "golden_common.hh"
#include "parse/ast_print.hh"
#include "parse/driver.hh"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace {

namespace fs = std::filesystem;

std::string renderFixture(const fs::path &fixturePath) {
  std::ostringstream errors;
  yy::Driver driver;

  int parseStatus = 0;
  {
    golden::ScopedCerrRedirect redirect(errors);
    parseStatus = driver.parse(fixturePath);
  }

  std::ostringstream output;
  output << "fixture: " << golden::displayPath(fixturePath).string() << "\n";
  output << "parse_status: " << parseStatus << "\n";
  output << "stderr:\n";

  const std::string errorText = errors.str();
  output << (errorText.empty() ? "<empty>\n" : errorText);

  if (parseStatus == 0) {
    output << "ast:\n";
    ast::print::rule(output, driver.getParsedRule());
    output << "\n";
  }

  return output.str();
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <parser-fixture.rule>\n";
    return 2;
  }

  const fs::path fixturePath = argv[1];
  return golden::verifyFixtureOutput(fixturePath, renderFixture(fixturePath));
}
