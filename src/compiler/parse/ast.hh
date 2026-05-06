#pragma once

#include "common/error.hh"
#include "common/util.hh"

#include <ghidra/opcodes.hh>
#include <ghidra/types.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ast {

template <class T> using Box = std::unique_ptr<T>;

class Id {
public:
  Id(std::string _name, size_t _num, bool _userDefined = false)
      : num(_num), name(_name), userDefined(_userDefined) {};
  Id() : Id("UNDEFINED", -1) {};

  std::weak_ordering operator<=>(const Id &other) const {
    std::weak_ordering res =
        other.userDefined <=>
        this->userDefined; // it's not typo; just Userdefined < NotUserdefined
    if (res == 0) {
      res = this->num <=> other.num;
    }

    return res;
  }

  bool operator==(const Id &other) const = default;

  size_t getNum() const { return num; };
  const std::string getName() const { return name; };

  size_t hash() const {
    size_t res = 0;
    hash_combine(res, this->getName());
    hash_combine(res, this->getNum());
    return res;
  }

private:
  size_t num;
  std::string name;
  bool userDefined = 0;
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
  const Id createId(std::string name, bool userDefined = false) {
    auto res = getIdByName(name);
    if (res.has_value()) {
      return res.value();
    } else {
      Id newId(name, idCount++, userDefined);
      map[name] = newId;
      return newId;
    }
  };

  /**
   * @brief Create new id
   * */
  const Id createId(bool userDefined = false) {
    return createId(
        "_" + anonymousPrefix + "" + std::to_string(anonymNameCount++),
        userDefined
    );
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
  std::optional<std::uint64_t> offset;
  // Identifiable<bool> isDefedByPnode;
};

struct VarnodeVar {
  Id id;
};

struct VarnodeVarWithType {
  VarnodeVar var;
  VarnodeType vntype;
};

struct VarnodeConst {
  int64_t value;
};

struct VarnodeEmpty {};

using VarnodeTerm =
    std::variant<VarnodeVar, VarnodeVarWithType, VarnodeEmpty, VarnodeConst>;

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

using OutVarnodeCondition = std::variant<
    OutVarnodeConditionDefault, OutVarnodeConditionNoOutOr,
    OutVarnodeConditionNoOut>;

const Size *getSizeOutCond(const OutVarnodeCondition &cond);

struct OpTypeScheme {
  InVarnodeConditions inVarnodeConds;
  OutVarnodeCondition outVarnodeCond;
  bool isMultiequal; // for now it's always false
};

struct OpType {
  std::string opName;
  OpTypeScheme scheme;
  ghidra::OpCode ghidraOpCode;

  std::weak_ordering operator<=>(const OpType &other) const {
    return this->ghidraOpCode <=> other.ghidraOpCode;
  }

  bool operator==(const OpType &other) const { return (*this <=> other) == 0; };
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
struct PnodeEmpty {};

using PnodeTerm = std::variant<PnodeVar, PnodeVarWithType, PnodeEmpty>;

struct VarnodeDefedBy;
struct PnodeThatTakeAsNthArg;
// struct PnodeThatTakeAsSomeArg;
struct BasicBlockDominatedBy;
struct BasicBlockOutgoingEdge;

using VarnodePattern = std::variant<
    VarnodeTerm,        // vname OR vname(conditions) OR EMPTY
    Box<VarnodeDefedBy> // pnode_patter -> vname
    >;
using PnodePattern = std::variant<
    PnodeTerm,                 // opname OR opname(conditions)
    Box<PnodeThatTakeAsNthArg> // vnode_pattern ->(N) opname
    //  Box<PnodeThatTakeAsSomeArg> // vnode_pattern -> opname
    >;

using BasicBlockPattern = std::variant<
    BasicBlockVar,              // bbname
    Box<BasicBlockDominatedBy>, // bb_pattern <= bbname
    Box<BasicBlockOutgoingEdge> // bb_pattern ->(N) bbname
    >;

struct VarnodeDefedBy {
  PnodePattern pp;
  VarnodeTerm v;
};

struct PnodeThatTakeAsNthArg {
  VarnodePattern vp;
  uint32_t num;
  PnodeTerm p;
};

// struct PnodeThatTakeAsSomeArg {
//   VarnodePattern vp;
//   PnodeTerm p;
// };

struct BasicBlockDominatedBy {
  BasicBlockPattern bbp;
  BasicBlockVar bb;
};

struct BasicBlockOutgoingEdge {
  BasicBlockPattern bbp;
  uint32_t outIndex;
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
    std::variant<VarnodeVar, VarnodeEmpty, VarnodeConst, VarnodeSpecSize>;
using PnodeActionTerm = std::variant<PnodeVar, PnodeSpecTypeAndLoc>;

struct EmptyActionDeletePnode {
  PnodeVar targetPnode;
};

struct VarnodeSetAsPnodeOut;
struct PnodeSetNthArg;

using VarnodeAction = std::variant<
    VarnodeActionTerm,        // vname OR new_vname(size)
                              // OR EMPTY
    Box<VarnodeSetAsPnodeOut> // pnode_action ->> vname
    >;

using PnodeAction = std::variant<
    PnodeActionTerm,    // opname OR opname(op BEFORE/AFTER old_opname)
    Box<PnodeSetNthArg> // vnode_action ->>(N) opname
    >;

using EmptyAction = std::variant<EmptyActionDeletePnode // DELETE opname
                                 >;

struct VarnodeSetAsPnodeOut {
  PnodeAction pa;
  VarnodeActionTerm v;
};

struct PnodeSetNthArg {
  VarnodeAction va;
  uint32_t num;
  PnodeActionTerm p;
};

using RuleAction = std::variant<VarnodeAction, PnodeAction, EmptyAction>;

struct Rule {
  std::vector<RulePattern> patterns;
  std::vector<RuleAction> actions;
};

} // namespace ast

namespace std {
template <> struct hash<ast::Id> {
  size_t operator()(const ast::Id &id) const { return id.hash(); }
};
} // namespace std
