#include "parse/driver.hh"
#include "rulecompile/rulecompile_driver.hh"
#include "rule_directory.hh"
#include "validate/validate_driver.hh"

#include <cereal/archives/portable_binary.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace fs = std::filesystem;

constexpr std::string_view DEFAULT_RULE_EXTENSION = ".pwrule";

struct CliOptions {
  std::vector<fs::path> inputs;
  std::optional<fs::path> output;
  std::optional<fs::path> outputDir;
  bool useRuleDirectory = false;
  bool checkOnly = false;
  bool verbose = false;
};

void printUsage(std::ostream &out, std::string_view program) {
  out << "Usage: " << program << " [options] <rule-file>...\n"
      << "\n"
      << "Options:\n"
      << "  -o, --output <file>      Write one compiled rule to <file>\n"
      << "  -d, --output-dir <dir>   Write compiled rules into <dir>\n"
      << "      --rules-dir          Write compiled rules into the plugin rule "
         "directory\n"
      << "      --check              Parse, validate, and compile without writing\n"
      << "  -v, --verbose            Print compile details\n"
      << "  -h, --help               Show this help\n"
      << "\n"
      << "The plugin rule directory is selected like the plugin does:\n"
      << "  PCODE_WEAVER_RULE_DIR, then XDG_DATA_HOME/pcode-weaver,\n"
      << "  then HOME/.local/share/pcode-weaver.\n";
}

std::optional<std::string>
parseArgs(int argc, char *argv[], CliOptions &options, bool &showHelp) {
  showHelp = false;
  opterr = 0;
  optind = 1;

  const option longOptions[] = {
      {"output", required_argument, nullptr, 'o'},
      {"output-dir", required_argument, nullptr, 'd'},
      {"rules-dir", no_argument, nullptr, 'r'},
      {"check", no_argument, nullptr, 'c'},
      {"verbose", no_argument, nullptr, 'v'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0},
  };

  while (true) {
    int longIndex = 0;
    const int opt = getopt_long(argc, argv, "o:d:vh", longOptions, &longIndex);
    if (opt == -1) {
      break;
    }

    switch (opt) {
    case 'h':
      showHelp = true;
      break;
    case 'v':
      options.verbose = true;
      break;
    case 'c':
      options.checkOnly = true;
      break;
    case 'r':
      options.useRuleDirectory = true;
      break;
    case 'o':
      options.output = fs::path{optarg};
      break;
    case 'd':
      options.outputDir = fs::path{optarg};
      break;
    case '?':
      return "invalid arguments";
    default:
      return "unknown option";
    }
  }

  for (int i = optind; i < argc; ++i) {
    options.inputs.emplace_back(argv[i]);
  }

  int outputModeCount = 0;
  outputModeCount += options.output.has_value() ? 1 : 0;
  outputModeCount += options.outputDir.has_value() ? 1 : 0;
  outputModeCount += options.useRuleDirectory ? 1 : 0;
  if (outputModeCount > 1) {
    return "choose only one of --output, --output-dir, or --rules-dir";
  }

  if (options.checkOnly && outputModeCount != 0) {
    return "--check cannot be combined with output options";
  }

  if (options.output.has_value() && options.inputs.size() > 1) {
    return "--output can be used with exactly one input rule";
  }

  if (!showHelp && options.inputs.empty()) {
    return "no input rule files";
  }

  return std::nullopt;
}

fs::path defaultOutputPath(const fs::path &input) {
  fs::path output = input.filename();
  output.replace_extension(DEFAULT_RULE_EXTENSION);

  if (input.has_parent_path()) {
    output = input.parent_path() / output;
  }

  return output;
}

fs::path outputPathFor(const CliOptions &options, const fs::path &input) {
  if (options.output.has_value()) {
    return *options.output;
  }

  fs::path output = input.filename();
  output.replace_extension(DEFAULT_RULE_EXTENSION);

  if (options.outputDir.has_value()) {
    return *options.outputDir / output;
  }

  if (options.useRuleDirectory) {
    return pcodeweaver::getRuleDirectory() / output;
  }

  return defaultOutputPath(input);
}

bool writeCompiledRule(
    const pcodeweaver::compiled::Rule &rule,
    const fs::path &path,
    std::string &error
) {
  try {
    if (path.has_parent_path()) {
      fs::create_directories(path.parent_path());
    }

    std::ofstream output(path, std::ios::out | std::ios::binary);
    if (!output.is_open()) {
      error = "failed to open output file";
      return false;
    }

    cereal::PortableBinaryOutputArchive archive(output);
    archive(rule);
    return true;
  } catch (const std::exception &ex) {
    error = ex.what();
    return false;
  }
}

Errorable<pcodeweaver::compiled::Rule> compileRule(const fs::path &input) {
  yy::Driver parseDriver;
  if (parseDriver.parse(input) != 0) {
    return err("parse failed");
  }

  const auto &rule = parseDriver.getParsedRule();
  ValidateDriver validateDriver(parseDriver.getContext());

  auto validateRes = validateDriver.validate(rule);
  if (!validateRes.has_value()) {
    return std::unexpected(validateRes.error());
  }

  auto runtimeChecks = validateDriver.getRuntimeCheckConditions();
  RuleCompileDriver compileDriver(RuleCompileInput{
      .actions = rule.actions,
      .patternGraph = validateDriver.getPGraph(),
      .actionGraph = validateDriver.getActGraph(),
      .sizeSolver = validateDriver.getSizeSolver(),
      .runtimeChecks = runtimeChecks,
      .runtimeValueRequirements = validateDriver.getRuntimeValueRequirements(),
  });

  return compileDriver.compile();
}

} // namespace

int main(int argc, char *argv[]) {
  CliOptions options;
  bool showHelp = false;
  auto argError = parseArgs(argc, argv, options, showHelp);
  if (showHelp) {
    printUsage(std::cout, argv[0]);
    return argError.has_value() ? 1 : 0;
  }

  if (argError.has_value()) {
    std::cerr << "error: " << *argError << "\n\n";
    printUsage(std::cerr, argv[0]);
    return 1;
  }

  bool hadError = false;
  for (const fs::path &input : options.inputs) {
    auto compileRes = compileRule(input);
    if (!compileRes.has_value()) {
      std::cerr << input.string() << ": " << compileRes.error().message()
                << "\n";
      hadError = true;
      continue;
    }

    if (options.verbose) {
      std::cout << input.string() << ": compiled "
                << compileRes->pattern.steps.size() << " pattern steps, "
                << compileRes->action.steps.size() << " action steps\n";
    }

    if (options.checkOnly) {
      continue;
    }

    const fs::path output = outputPathFor(options, input);
    std::string error;
    if (!writeCompiledRule(*compileRes, output, error)) {
      std::cerr << input.string() << ": failed to write " << output.string()
                << ": " << error << "\n";
      hadError = true;
      continue;
    }

    if (options.verbose || options.inputs.size() > 1 ||
        options.useRuleDirectory || options.outputDir.has_value()) {
      std::cout << input.string() << " -> " << output.string() << "\n";
    }
  }

  return hadError ? 1 : 0;
}
