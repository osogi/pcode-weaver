#pragma once

#include "common/util.hh"
#include "common/error.hh"

#include <ghidra/types.h>
#include <ghidra/opcodes.hh>

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ast {

template <class T> using Box = std::unique_ptr<T>;

class Id {
public:
  Id() : num(-1), name("UNDEFINED") {};
  Id(std::string _name, size_t _num) : num(_num), name(_name) {};

  size_t getNum() { return num; };
  const std::string getName() const { return name; };

private:
  size_t num;
  std::string name;
};

class IdFactory {
public:
  IdFactory(std::string prefix) : anonymousPrefix(prefix) {};

  /**
   * @brief Get already created id by name
   * */
  Errorable<Id> getIdByName(std::string name) {
    auto res = map.find(name);
    if (res != map.end()) {
      return res->second;
    } else {
      return err("Id not found");
    }
  };

  /**
   * @brief Get created or create id with target name
   * */
  const Id createId(std::string name) {
    auto res = getIdByName(name);
    if (res.has_value()) {
      return res.value();
    } else {
      Id newId(name, idCount++);
      map[name] = newId;
      return newId;
    }
  };

  /**
   * @brief Create new id
   * */
  const Id createId() {
    return createId("_" + anonymousPrefix + "" + std::to_string(anonymNameCount++));
  };

private:
  size_t idCount = 0;
  size_t anonymNameCount = 0;

  std::string anonymousPrefix;
  std::unordered_map<std::string, Id> map;
};

template <typename CONCRETE_VALUE_TYPE>
using Identifiable = std::variant<Id, CONCRETE_VALUE_TYPE>;

using Size = Identifiable<ghidra::int4>;

struct BasicBlockVar {
  Id id;
};

struct VarnodeType {
  Size size;
  BasicBlockVar declarationBB;
  // Identifiable<bool> isDefedByPnode;
};

struct VarnodeVar {
  Id id;
};

struct VarnodeVarWithType {
  VarnodeVar var;
  VarnodeType vntype;
};

struct VarnodeEmpty {};

using VarnodeTerm = std::variant<VarnodeVar, VarnodeVarWithType, VarnodeEmpty>;

struct InVarnodeCondition {
  Size size;
};
struct InVarnodeConditionsArray {
  std::vector<InVarnodeCondition> array;
};

struct InVarnodeConditionsSpecial {};

using InVarnodeConditions =
    std::variant<InVarnodeConditionsArray, InVarnodeConditionsSpecial>;

struct OutVarnodeConditionDefault {
  Size size;
};

struct OutVarnodeConditionNoOutOr {
  OutVarnodeConditionDefault alt;
};

struct OutVarnodeConditionNoOut {};

using OutVarnodeCondition =
    std::variant<OutVarnodeConditionDefault, OutVarnodeConditionNoOutOr,
                 OutVarnodeConditionNoOut>;

struct OpTypeScheme {
  InVarnodeConditions inVarnodeConds;
  OutVarnodeCondition outVarnodeCond;
  bool isMultiequal; // for now it's always false
};

struct OpType {
  std::string opName;
  OpTypeScheme scheme;
  ghidra::OpCode ghidraOpCode;
};

struct PnodeType {
  OpType optype;
  BasicBlockVar bb;
};

struct PnodeVar {
  Id id;
};
struct PnodeVarWithType {
  PnodeVar var;
  PnodeType ptype;
};

using PnodeTerm = std::variant<PnodeVar, PnodeVarWithType>;

struct VarnodeDefedBy;
struct PnodeThatTakeAsNthArg;
struct PnodeThatTakeAsSomeArg;
struct BasicBlockDominatedBy;

using VarnodePattern =
    std::variant<VarnodeTerm,        // vname OR vname(conditions) OR EMPTY
                 Box<VarnodeDefedBy> // pnode_patter -> vname
                 >;
using PnodePattern =
    std::variant<PnodeTerm,                  // opname OR opname(conditions)
                 Box<PnodeThatTakeAsNthArg>, // vnode_pattern ->(N) opname
                 Box<PnodeThatTakeAsSomeArg> // vnode_pattern -> opname
                 >;

using BasicBlockPattern =
    std::variant<BasicBlockVar,             // bbname
                 Box<BasicBlockDominatedBy> //  bb_pattern <= bbname
                 >;

struct VarnodeDefedBy {
  PnodePattern pp;
  VarnodeTerm v;
};

struct PnodeThatTakeAsNthArg {
  VarnodePattern vp;
  ghidra::int4 num;
  PnodeTerm p;
};

struct PnodeThatTakeAsSomeArg {
  VarnodePattern vp;
  PnodeTerm p;
};

struct BasicBlockDominatedBy {
  BasicBlockPattern bbp;
  BasicBlockVar bb;
};

using RulePattern =
    std::variant<VarnodePattern, PnodePattern, BasicBlockPattern>;

// Action

struct PnodeSpecTypeAndLoc {
  PnodeVar newVar;
  OpType opType;
  bool isInsertBefore; // True - Before; False - After
  PnodeVar oldVar;
};

struct VarnodeSpecSize {
  VarnodeVar newVar;
  Size size;
};

using VarnodeActionTerm =
    std::variant<VarnodeVar, VarnodeEmpty, VarnodeSpecSize>;
using PnodeActionTerm = std::variant<PnodeVar, PnodeSpecTypeAndLoc>;

struct VarnodeSetAsPnodeOut;
struct PnodeSetNthArg;

using VarnodeAction =
    std::variant<VarnodeActionTerm, // vname OR new_vname(size)
                                    // OR EMPTY
                 Box<VarnodeSetAsPnodeOut> // pnode_action ->> vname
                 >;

using PnodeAction =
    std::variant<PnodeActionTerm,    // opname OR opname(op BEFORE/AFTER old_opname)
                 Box<PnodeSetNthArg> // vnode_action ->>(N) opname
                 >;

struct VarnodeSetAsPnodeOut {
  PnodeAction pa;
  VarnodeActionTerm v;
};

struct PnodeSetNthArg {
  VarnodeAction va;
  ghidra::int4 num;
  PnodeActionTerm p;
};

using RuleAction = std::variant<VarnodeAction, PnodeAction>;

struct Rule {
  std::vector<RulePattern> patterns;
  std::vector<RuleAction> actions;
};

} // namespace ast
