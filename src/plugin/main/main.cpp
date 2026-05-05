// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "compiled_rule.hh"
#include "pwrule.hh"

#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <ghidra/action.hh>
#include <ghidra/funcdata.hh>
#include <reoxide/logging.hh>
#include <reoxide/reoxide_interface.hh>
#include <reoxide/reoxide_plugin.hh>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace ghidra;
using namespace reoxide;

#if defined(PCODE_WEAVER_DEBUG) || defined(DEBUG)
#define PCODE_WEAVER_DEBUG_LOG(...) LOG_DEBUG(__VA_ARGS__)
#define PCODE_WEAVER_DEBUG_SEND(reox, message) (reox).sendString(message)
#else
#define PCODE_WEAVER_DEBUG_LOG(...)                                            \
  do {                                                                         \
  } while (0)
#define PCODE_WEAVER_DEBUG_SEND(reox, message)                                 \
  do {                                                                         \
  } while (0)
#endif

namespace {

constexpr std::uint32_t CURRENT_RULE_FORMAT_VERSION = 1;

struct LoadedRule {
  std::filesystem::path path;
  pcodeweaver::compiled::Rule rule;
};

std::filesystem::path getRuleDirectory() {
  const char *envRuleDir = std::getenv("PCODE_WEAVER_RULE_DIR");
  if (envRuleDir != nullptr && envRuleDir[0] != '\0') {
    return envRuleDir;
  }

  const char *xdgDataHome = std::getenv("XDG_DATA_HOME");
  if (xdgDataHome != nullptr && xdgDataHome[0] != '\0') {
    return std::filesystem::path{xdgDataHome} / "pcode-weaver";
  }

  const char *home = std::getenv("HOME");
  if (home != nullptr && home[0] != '\0') {
    return std::filesystem::path{home} / ".local" / "share" / "pcode-weaver";
  }

  return std::filesystem::path{".local"} / "share" / "pcode-weaver";
}

template <class Archive>
std::optional<pcodeweaver::compiled::Rule> loadRuleWithArchive(
    const std::filesystem::path &path, std::ios::openmode mode
) {
  std::ifstream input(path, mode);
  if (!input.is_open()) {
    return std::nullopt;
  }

  pcodeweaver::compiled::Rule rule;
  Archive archive(input);
  archive(rule);
  return rule;
}

std::optional<pcodeweaver::compiled::Rule>
loadCompiledRule(const std::filesystem::path &path, std::string &error) {
  try {
    try {
      return loadRuleWithArchive<cereal::PortableBinaryInputArchive>(
          path, std::ios::in | std::ios::binary
      );
    } catch (const std::exception &portableError) {
      LOG_INFO(
          "Portable binary load failed for %s: %s",
          path.c_str(),
          portableError.what()
      );
    }

    return loadRuleWithArchive<cereal::BinaryInputArchive>(
        path, std::ios::in | std::ios::binary
    );
  } catch (const std::exception &ex) {
    error = ex.what();
  }

  return std::nullopt;
}

std::vector<std::filesystem::path>
listRuleFiles(const std::filesystem::path &ruleDirectory) {
  std::vector<std::filesystem::path> paths;
  std::error_code error;

  const bool exists = std::filesystem::exists(ruleDirectory, error);
  if (error) {
    LOG_ERROR(
        "Failed to check PcodeWeaver rule directory %s: %s",
        ruleDirectory.c_str(),
        error.message().c_str()
    );
    return paths;
  }

  if (!exists) {
    LOG_ERROR(
        "PcodeWeaver rule directory does not exist: %s", ruleDirectory.c_str()
    );
    return paths;
  }

  const bool isDirectory = std::filesystem::is_directory(ruleDirectory, error);
  if (error) {
    LOG_ERROR(
        "Failed to inspect PcodeWeaver rule directory %s: %s",
        ruleDirectory.c_str(),
        error.message().c_str()
    );
    return paths;
  }

  if (!isDirectory) {
    LOG_ERROR(
        "PcodeWeaver rule path is not a directory: %s", ruleDirectory.c_str()
    );
    return paths;
  }

  std::filesystem::directory_iterator iter(ruleDirectory, error);
  if (error) {
    LOG_ERROR(
        "Failed to open PcodeWeaver rule directory %s: %s",
        ruleDirectory.c_str(),
        error.message().c_str()
    );
    return paths;
  }

  for (const std::filesystem::directory_iterator end; iter != end;
       iter.increment(error)) {
    if (error) {
      LOG_ERROR(
          "Failed to read PcodeWeaver rule directory %s: %s",
          ruleDirectory.c_str(),
          error.message().c_str()
      );
      break;
    }

    if (iter->is_regular_file(error)) {
      paths.push_back(iter->path());
    }

    if (error) {
      LOG_ERROR(
          "Failed to inspect PcodeWeaver rule path %s: %s",
          iter->path().c_str(),
          error.message().c_str()
      );
      error.clear();
    }
  }

  std::sort(paths.begin(), paths.end());
  return paths;
}

} // namespace

class PcodeWeaverPlugin : public reoxide::Plugin {
public:
  PcodeWeaverPlugin() {
    LOG_INFO("Initializing PcodeWeaverPlugin");
    loadRules();
  }

  virtual ~PcodeWeaverPlugin() { LOG_INFO("Destructing PcodeWeaverPlugin"); }

  const std::vector<LoadedRule> &getRules() const { return rules; }

private:
  std::vector<LoadedRule> rules;

  void loadRules() {
    const std::filesystem::path ruleDirectory = getRuleDirectory();
    LOG_INFO("Loading PcodeWeaver rules from %s", ruleDirectory.c_str());

    for (const std::filesystem::path &path : listRuleFiles(ruleDirectory)) {
      std::string error;
      std::optional<pcodeweaver::compiled::Rule> rule =
          loadCompiledRule(path, error);
      if (!rule.has_value()) {
        LOG_ERROR(
            "Failed to load PcodeWeaver rule %s: %s",
            path.c_str(),
            error.empty() ? "unknown format" : error.c_str()
        );
        continue;
      }

      if (rule->formatVersion != CURRENT_RULE_FORMAT_VERSION) {
        LOG_ERROR(
            "Skipping PcodeWeaver rule %s: unsupported format version %u",
            path.c_str(),
            rule->formatVersion
        );
        continue;
      }

      LOG_INFO(
          "Loaded PcodeWeaver rule %s: %zu pattern steps, %zu action steps",
          path.c_str(),
          rule->pattern.steps.size(),
          rule->action.steps.size()
      );
      rules.push_back(LoadedRule{path, std::move(*rule)});
    }

    LOG_INFO("Loaded %zu PcodeWeaver rules", rules.size());
  }
};

class PcodeWeaverAction : public Action {
public:
  PcodeWeaverAction(
      const string &group, ReOxideInterface &reox, PcodeWeaverPlugin &plugin
  )
      : Action(0, "pcodeweaver", group), reox{reox}, plugin{plugin} {
    LOG_INFO("Initializing pcodeweaver action");
  }

  virtual Action *clone(const ActionGroupList &grouplist) const override {
    if (!grouplist.contains(getGroup())) {
      return nullptr;
    }
    return new PcodeWeaverAction{getGroup(), reox, plugin};
  }

  virtual int4 apply(Funcdata &data) override {
    int4 appliedRules = 0;

    for (const LoadedRule &loadedRule : plugin.getRules()) {
      PcodeWeaverRule rule{loadedRule.rule};
      const int4 applied = rule.apply(data);
      if (applied == 0) {
        continue;
      }

      appliedRules += applied;
      PCODE_WEAVER_DEBUG_LOG(
          "Applied PcodeWeaver rule %s to %s",
          loadedRule.path.c_str(),
          data.getName().c_str()
      );
      PCODE_WEAVER_DEBUG_SEND(
          reox,
          "pcodeweaver applied " + loadedRule.path.filename().string() +
              " to " + data.getName()
      );
    }

    return appliedRules;
  }

private:
  ReOxideInterface &reox;
  PcodeWeaverPlugin &plugin;
};

static Action *new_PcodeWeaverAction(
    const reoxide::Context *ctx, reoxide::Plugin *pluginContext,
    const reoxide::InitArgs *args
) {
  PcodeWeaverPlugin *plugin = dynamic_cast<PcodeWeaverPlugin *>(pluginContext);
  if (plugin == nullptr) {
    LOG_ERROR("PcodeWeaver action received an invalid plugin context");
    return nullptr;
  }
  return new PcodeWeaverAction(args->group_name, *ctx->reoxide, *plugin);
}

REOXIDE_CONTEXT(PcodeWeaverPlugin);
REOXIDE_ACTIONS({"pcodeweaver", new_PcodeWeaverAction})
REOXIDE_RULES()
