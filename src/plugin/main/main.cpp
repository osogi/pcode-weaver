// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2025 Michael Pucher <contact@cluo.sh>
#include <filesystem>
#include <ghidra/action.hh>
#include <ghidra/funcdata.hh>
#include <reoxide/logging.hh>
#include <reoxide/reoxide_interface.hh>
#include <reoxide/reoxide_plugin.hh>
#include <unistd.h>

using namespace ghidra;
using namespace reoxide;

#define RULE_DIR "/home/user1/.local/share/pcode-weaver"

class PcodeWeaverPlugin : public reoxide::Plugin {

  std::unordered_map<std::string, std::string>

  readDirectoryFiles(const std::string &directoryPath) {
    std::unordered_map<std::string, std::string> fileContents;
    std::filesystem::path dir(directoryPath);

    // Check if the directory exists and is actually a directory
    if (!std::filesystem::exists(dir)) {
      LOG_ERROR("Dir don't exist");

      return fileContents; // Return empty map
    }
    if (!std::filesystem::is_directory(dir)) {
      LOG_ERROR("dir is not a dir");

      return fileContents; // Return empty map
    }

    // Iterate over all entries in the directory
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      // Check if the current entry is a regular file
      if (std::filesystem::is_regular_file(entry.status())) {
        std::string filename =
            entry.path().filename().string();         // Get just the filename
        std::string filePath = entry.path().string(); // Get the full file path

        std::ifstream inputFile(filePath); // Open the file

        if (inputFile.is_open()) {
          // Read the entire content of the file into a stringstream
          std::stringstream buffer;
          buffer << inputFile.rdbuf();
          std::string content = buffer.str();

          // Store the filename and its content in the map
          fileContents[filename] = content;
          LOG_INFO("Read counter: %s", filename.c_str());

          inputFile.close(); // Close the file
        }
      }
      // You can add an 'else if
      // (std::filesystem::is_directory(entry.status()))' here if you want to
      // handle subdirectories (e.g., recursively)
    }

    return fileContents;
  }

public:
  std::unordered_map<std::string, std::string> rules;

  PcodeWeaverPlugin() {
    // It is not possible to communicate with the ReOxide manager
    // during initialization of the plugin, but we can write to the
    // the reoxide.log file.
    LOG_INFO("Initializing PcodeWeaverPlugin");
    std::string cwd = std::filesystem::current_path();
    rules = readDirectoryFiles(RULE_DIR);
    LOG_INFO("Work dir %s", cwd.c_str());
  }

  virtual ~PcodeWeaverPlugin() { LOG_INFO("Destructing PcodeWeaverPlugin"); }

private:
  int counter = 0;
};

class SimpleAction : public Action {
public:
  SimpleAction(const string &g, ReOxideInterface &reox, PcodeWeaverPlugin &plugin)
      : Action(0, "simpleaction", g), reox{reox}, simple_plugin{plugin} {
    LOG_INFO("Initializing simpleaction");
  }

  virtual Action *clone(const ActionGroupList &grouplist) const override {
    if (!grouplist.contains(getGroup()))
      return nullptr;
    return new SimpleAction{getGroup(), reox, simple_plugin};
  }

  virtual int4 apply(Funcdata &data) override {
    reox.sendString("simple_action triggered for " + data.getName());
    data.warningHeader("SimpleAction was applied on this function.");
    for (const auto &[k, v] : simple_plugin.rules) {
      std::stringstream s;
      s << "RuleAction:" << k << " = " << v;
      reox.sendString(s.str());
    }
    return 0;
  }

private:
  ReOxideInterface &reox;
  PcodeWeaverPlugin &simple_plugin;
};

/// This is the constructor for the action as called from ReOxide
static Action *new_SimpleAction(
    const reoxide::Context *ctx, reoxide::Plugin *plugin_context,
    const reoxide::InitArgs *args
) {
  PcodeWeaverPlugin *plugin = dynamic_cast<PcodeWeaverPlugin *>(plugin_context);
  return new SimpleAction(args->group_name, *ctx->reoxide, *plugin);
}

REOXIDE_CONTEXT(PcodeWeaverPlugin);
REOXIDE_ACTIONS({"simpleaction", new_SimpleAction})
REOXIDE_RULES({})
