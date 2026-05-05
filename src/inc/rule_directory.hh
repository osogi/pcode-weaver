#pragma once

#include <cstdlib>
#include <filesystem>

namespace pcodeweaver {

inline std::filesystem::path getRuleDirectory() {
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

} // namespace pcodeweaver
