#include "golden_common.hh"

#include "ApprovalTests.hpp"
#include "ApprovalTests/core/ApprovalNamer.h"

#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <utility>

namespace golden {

namespace fs = std::filesystem;

namespace {

class FixtureAdjacentNamer : public ApprovalTests::ApprovalNamer {
public:
  explicit FixtureAdjacentNamer(fs::path fixturePath)
      : outputBase(std::move(fixturePath)) {
    outputBase.replace_extension();
  }

  std::string getApprovedFile(std::string extensionWithDot) const override {
    return pathFor("approved", extensionWithDot);
  }

  std::string getReceivedFile(std::string extensionWithDot) const override {
    return pathFor("received", extensionWithDot);
  }

private:
  std::string
  pathFor(const std::string &kind, const std::string &extensionWithDot) const {
    fs::path output = outputBase;
    output += "." + kind + extensionWithDot;
    return output.string();
  }

  fs::path outputBase;
};

} // namespace

ScopedCerrRedirect::ScopedCerrRedirect(std::ostream &replacement)
    : originalBuffer(std::cerr.rdbuf(replacement.rdbuf())) {}

ScopedCerrRedirect::~ScopedCerrRedirect() { std::cerr.rdbuf(originalBuffer); }

fs::path displayPath(const fs::path &fixturePath) {
  return fs::relative(fixturePath, fs::current_path());
}

int verifyFixtureOutput(
    const fs::path &fixturePath, const std::string &output
) {
  try {
    static const ApprovalTests::QuietReporter quietReporter;
    ApprovalTests::Approvals::verify(
        output,
        ApprovalTests::Options()
            .withReporter(quietReporter)
            .withNamer(std::make_shared<FixtureAdjacentNamer>(fixturePath))
    );
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << "\n";
    return 1;
  }
}

} // namespace golden
