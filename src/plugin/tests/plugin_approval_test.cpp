// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "ApprovalTests/ApprovalTests.hpp"
#include "ApprovalTests/core/ApprovalNamer.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <utility>

namespace {

namespace fs = std::filesystem;

class CaseNamer : public ApprovalTests::ApprovalNamer {
public:
  explicit CaseNamer(fs::path caseDir) : caseDir(std::move(caseDir)) {}

  std::string getApprovedFile(std::string) const override {
    return (caseDir / "approved" / "main.approved.txt").string();
  }

  std::string getReceivedFile(std::string) const override {
    return (caseDir / "approved" / "main.received.txt").string();
  }

private:
  fs::path caseDir;
};

std::string shellQuote(const fs::path &path) {
  std::string input = path.string();
  std::string quoted = "'";
  for (const char ch : input) {
    if (ch == '\'') {
      quoted += "'\\''";
    } else {
      quoted += ch;
    }
  }
  quoted += "'";
  return quoted;
}

std::string shellQuote(const std::string &input) {
  std::string quoted = "'";
  for (const char ch : input) {
    if (ch == '\'') {
      quoted += "'\\''";
    } else {
      quoted += ch;
    }
  }
  quoted += "'";
  return quoted;
}

std::string getenvOr(std::string_view name, std::string fallback) {
  const char *value = std::getenv(std::string(name).c_str());
  if (value == nullptr || *value == '\0') {
    return fallback;
  }
  return value;
}

std::string runCase(const fs::path &caseDir) {
  const fs::path script = fs::current_path() / "scripts" / "run-docker-case.sh";
  const std::string image = getenvOr(
      "PCODE_WEAVER_PLUGIN_TEST_IMAGE", "pcode-weaver-plugin-test:latest"
  );
  const std::string command = shellQuote(script) + " " +
                              shellQuote(fs::absolute(caseDir)) + " " +
                              shellQuote(image);

  std::array<char, 4096> buffer{};
  std::string output;

  FILE *pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    throw std::runtime_error("failed to start plugin test runner");
  }

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
         nullptr) {
    output += buffer.data();
  }

  const int status = pclose(pipe);
  if (status == -1) {
    throw std::runtime_error("failed to close plugin test runner");
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    throw std::runtime_error(
        "plugin test runner failed for " + caseDir.string()
    );
  }

  return output;
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <case-dir>\n";
    return EXIT_FAILURE;
  }

  const fs::path caseDir = argv[1];

  try {
    static const ApprovalTests::QuietReporter quietReporter;
    ApprovalTests::Approvals::verify(
        runCase(caseDir),
        ApprovalTests::Options()
            .withReporter(quietReporter)
            .withNamer(std::make_shared<CaseNamer>(caseDir))
    );
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
