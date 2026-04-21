// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2025 Michael Pucher <contact@cluo.sh>
#include <ghidra/action.hh>
#include <ghidra/funcdata.hh>
#include <reoxide/logging.hh>
#include <reoxide/reoxide_interface.hh>
#include <reoxide/reoxide_plugin.hh>

using namespace ghidra;
using namespace reoxide;

/// This is the main plugin that is initialized. The rules and actions
/// are allowed to store information based on the Funcdata object they
/// are working on. The plugin allows storing information in a more
/// global way, i.e. across rule and action lifetime. Be aware that
/// Ghidra might spawn multiple decompiler instances for the same
/// program, so currently it is not possible to store information for
/// use across functions in a consistent manner. Therefore, some
/// ideas for current use cases are limited to e.g.:
///
/// * Statistics
/// * Shared data storage across rules/actions
class SimplePlugin : public reoxide::Plugin {
public:
  SimplePlugin() {
    // It is not possible to communicate with the ReOxide manager
    // during initialization of the plugin, but we can write to the
    // the reoxide.log file.
    LOG_INFO("Initializing SimplePlugin");
  }

  virtual ~SimplePlugin() { LOG_INFO("Destructing SimplePlugin"); }

  /// Simple demo function for the information storing. We just
  /// increase a counter and log its result. NOTE: This is threadsafe
  /// because the whole decompilation pipeline is single-threaded
  /// and it's probably impossible to parallelize it without completely
  /// rewriting Ghidra.
  void increase_counter() {
    counter++;
    LOG_INFO("SimplePlugin counter: %d", counter);
  }

private:
  int counter = 0;
};

/// Here we implement the Rule interface from Ghidra. This is about as
/// minimal as it can be, we need to properly initialize it and
/// implement the clone, getOpList and applyOp functions.
/// Initialization and clone always follow the same pattern.
class SimpleRule : public Rule {
public:
  SimpleRule(const string &g, ReOxideInterface &reox, SimplePlugin &plugin)
      : Rule(g, 0, "simple"), reox(reox), simple_plugin{plugin} {
    // We have the ReOxide object available here now, so we could in
    // theory call "send_string" to communicate with the manager, but
    // this is currently not possible during initialization (if you
    // try to do so, it will currently crash the decompiler!). But as
    // previously, we can write to the reoxide.log file.
    LOG_INFO("Initializing simple");
  }

  /// This always needs to be implemented this way. All rules are
  /// first registered in the universal action and for different
  /// commands, Ghidra extracts the rules to be executed and clones
  /// them. Due to the constructor having to be called, there's
  /// currently no better way than to just copy it for every rule.
  virtual Rule *clone(const ActionGroupList &grouplist) const {
    if (!grouplist.contains(getGroup()))
      return (Rule *)0;
    return new SimpleRule{getGroup(), reox, simple_plugin};
  }

  /// Specifying the P-Code opcodes this rule should apply on
  virtual void getOpList(vector<uint4> &oplist) const {
    oplist.push_back(CPUI_CALL);
  }

  /// This is called when Ghidra tries to apply your rule. Returning
  /// 1 signifies a change in state and that your rule applied,
  /// meaning Ghidra will try to re-run the Rule pool to see if any
  /// other rules apply (which could potentially result in loops).
  /// Returning 0 means that the rule did not apply.
  virtual int4 applyOp(PcodeOp *op, Funcdata &data) {
    Varnode *vn = op->getIn(0);

    // We apply our rule to all call operations, so the varnode should
    // be a function call specification
    AddrSpace *spc = vn->getSpace();
    if (spc->getType() != IPTR_FSPEC)
      return 0;

    // If the address space is IPTR_FSPEC, then we should be
    // able to cast the offset to an actual function spec object
    FuncCallSpecs *fc = reinterpret_cast<FuncCallSpecs *>(vn->getOffset());

    // Just do something with simple_plugin as test
    simple_plugin.increase_counter();

    // We can send the name of the called function to the ReOxide
    // manager. Currently, the manager cannot do much more
    reox.sendString(fc->getName());
    return 0;
  }

private:
  ReOxideInterface &reox;
  SimplePlugin &simple_plugin;
};

/// This is the constructor for the rule as called from ReOxide
static Rule *new_SimpleRule(const reoxide::Context *ctx,
                            reoxide::Plugin *plugin_context,
                            const reoxide::InitArgs *args) {
  SimplePlugin *plugin = dynamic_cast<SimplePlugin *>(plugin_context);
  return new SimpleRule(args->group_name, *ctx->reoxide, *plugin);
}

/// The definition of an action is governed by the same rules as
/// defining a new rule, w.r.t. constructor and clone function.
/// This time we don't specify an opcode to match on, as the
/// action is meant to perform transforms across the whole function.
class SimpleAction : public Action {
public:
  SimpleAction(const string &g, ReOxideInterface &reox, SimplePlugin &plugin)
      : Action(0, "simpleaction", g), reox{reox}, simple_plugin{plugin} {
    LOG_INFO("Initializing simpleaction");
  }

  virtual Action *clone(const ActionGroupList &grouplist) const override {
    if (!grouplist.contains(getGroup()))
      return nullptr;
    return new SimpleAction{getGroup(), reox, simple_plugin};
  }

  virtual int4 apply(Funcdata &data) override {
    AppInfo.getActiveProject().getProjectLocator().getMarkerFile();
    reox.sendString("simple_action triggered for " + data.getName());
    data.warningHeader("SimpleAction was applied on this function.");
    return 0;
  }

private:
  ReOxideInterface &reox;
  SimplePlugin &simple_plugin;
};

/// This is the constructor for the action as called from ReOxide
static Action *new_SimpleAction(const reoxide::Context *ctx,
                                reoxide::Plugin *plugin_context,
                                const reoxide::InitArgs *args) {
  SimplePlugin *plugin = dynamic_cast<SimplePlugin *>(plugin_context);
  return new SimpleAction(args->group_name, *ctx->reoxide, *plugin);
}

// The plugin context, the rules and the actions need to be registered
// using the following macros. The simple rule is registered under
// different names to show the syntax for registering multiple rules.
REOXIDE_CONTEXT(SimplePlugin);
REOXIDE_RULES({"simple", new_SimpleRule}, {"basic", new_SimpleRule})
REOXIDE_ACTIONS({"simpleaction", new_SimpleAction})
