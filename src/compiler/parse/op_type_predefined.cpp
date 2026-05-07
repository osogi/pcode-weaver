// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "parse/op_type_predefined.hh"

// generated headers
#include "parse/parser.hh"

#include <cassert>
#include <unordered_map>

using namespace ast;

static IdFactory helpIdFactory("_predef_op_tp_");

Size createSize(const std::string &varName) {
  if (varName == "_") {
    return helpIdFactory.createId(false);
  }
  return helpIdFactory.createId(varName, false);
}

// useless ?
Size createSize(int32_t val) { return static_cast<ghidra::int4>(val); }

InVarnodeConditions createInConds(const std::vector<std::string> &strArr) {
  InVarnodeConditionsArray resConds;
  resConds.array.resize(strArr.size());
  for (unsigned i = 0; i < strArr.size(); i++) {
    resConds.array[i] = InVarnodeCondition{createSize(strArr[i])};
  }
  return resConds;
}

InVarnodeConditions createInConds(const std::string &str) {
  assert(str == "*");
  return InVarnodeConditionsSpecial{};
}

#define OP_WITH_OPCODE(token, inConds, outCond, opcode)                        \
  (std::pair<OpTypeToken, OpType>{                                             \
      yy::Parser::token_kind_type::RULES_TOKEN_##token,                        \
      OpType{                                                                  \
          #token,                                                              \
          OpTypeScheme{inConds, outCond, false},                               \
          opcode                                                               \
      }                                                                        \
  })

#define OP(token, inConds, outCond)                                            \
  OP_WITH_OPCODE(token, inConds, outCond, ghidra::OpCode::CPUI_##token)

#define OP_CUSTOM_OUT(name, inCondsRaw, outCond)                               \
  OP(name, createInConds(inCondsRaw), outCond)

#define OP_CUSTOM_OPCODE(name, inCondsRaw, outCondRaw, opcode)                 \
  OP_WITH_OPCODE(                                                              \
      name, createInConds(inCondsRaw),                                         \
      OutVarnodeConditionDefault{createSize(outCondRaw)}, opcode               \
  )

#define OP_DEFAULT(name, inCondsRaw, outCondRaw)                               \
  OP_CUSTOM_OUT(                                                               \
      name, inCondsRaw, OutVarnodeConditionDefault{createSize(outCondRaw)}     \
  )

#define OP_NO_OUT(name, inCondsRaw)                                            \
  OP_CUSTOM_OUT(name, inCondsRaw, OutVarnodeConditionNoOut{})

#define OP_NO_OUT_CUSTOM_OPCODE(name, inCondsRaw, opcode)                      \
  OP_WITH_OPCODE(                                                              \
      name, createInConds(inCondsRaw), OutVarnodeConditionNoOut{}, opcode      \
  )

#define OP_NO_OUT_OR(name, inCondsRaw, outCondRaw)                             \
  OP_CUSTOM_OUT(                                                               \
      name, inCondsRaw,                                                        \
      OutVarnodeConditionNoOutOr{                                              \
          OutVarnodeConditionDefault{createSize(outCondRaw)}                   \
      }                                                                        \
  )

#define OP_NO_OUT_OR_CUSTOM_OPCODE(name, inCondsRaw, outCondRaw, opcode)       \
  OP_WITH_OPCODE(                                                              \
      name, createInConds(inCondsRaw),                                         \
      OutVarnodeConditionNoOutOr{                                              \
          OutVarnodeConditionDefault{createSize(outCondRaw)}                   \
      },                                                                       \
      opcode                                                                   \
  )

#define STRVEC(...) (std::vector<std::string>{__VA_ARGS__})

// ########## DEFINING OP TYPES ###################33

std::unordered_map<OpTypeToken, OpType> defaultOpType = {
    OP_DEFAULT(COPY, STRVEC("a"), "a"),
    OP_DEFAULT(LOAD, STRVEC("_", "_"), "_"),
    OP_NO_OUT(STORE, STRVEC("_", "_", "_")),

    OP_NO_OUT(BRANCH, STRVEC("_")),
    OP_NO_OUT(CBRANCH, STRVEC("_", "1")),
    OP_NO_OUT(BRANCHIND, STRVEC("_")),
    OP_NO_OUT_OR(CALL, "*", "_"),
    OP_NO_OUT_OR(CALLIND, "*", "_"),
    OP_NO_OUT_OR_CUSTOM_OPCODE(USERDEFINED, "*", "_", ghidra::OpCode::CPUI_CALLOTHER),
    OP_NO_OUT(RETURN, "*"),

    OP_DEFAULT(PIECE, STRVEC("_", "_"), "_"),
    OP_DEFAULT(SUBPIECE, STRVEC("_", "_"), "_"),
    OP_DEFAULT(POPCOUNT, STRVEC("_"), "_"),
    OP_DEFAULT(LZCOUNT, STRVEC("_"), "_"),

    OP_DEFAULT(INT_EQUAL, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_NOTEQUAL, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_LESS, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_SLESS, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_LESSEQUAL, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_SLESSEQUAL, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_ZEXT, STRVEC("_"), "_"),
    OP_DEFAULT(INT_SEXT, STRVEC("_"), "_"),
    OP_DEFAULT(INT_ADD, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_SUB, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_CARRY, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_SCARRY, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_SBORROW, STRVEC("a", "a"), "1"),
    OP_DEFAULT(INT_2COMP, STRVEC("a"), "a"),
    OP_DEFAULT(INT_NEGATE, STRVEC("a"), "a"),
    OP_DEFAULT(INT_XOR, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_AND, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_OR, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_LEFT, STRVEC("a", "_"), "a"),
    OP_DEFAULT(INT_RIGHT, STRVEC("a", "_"), "a"),
    OP_DEFAULT(INT_SRIGHT, STRVEC("a", "_"), "a"),
    OP_DEFAULT(INT_MULT, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_DIV, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_REM, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_SDIV, STRVEC("a", "a"), "a"),
    OP_DEFAULT(INT_SREM, STRVEC("a", "a"), "a"),

    OP_DEFAULT(BOOL_NEGATE, STRVEC("1"), "1"),
    OP_DEFAULT(BOOL_XOR, STRVEC("1", "1"), "1"),
    OP_DEFAULT(BOOL_AND, STRVEC("1", "1"), "1"),
    OP_DEFAULT(BOOL_OR, STRVEC("1", "1"), "1"),

    OP_DEFAULT(FLOAT_EQUAL, STRVEC("a", "a"), "1"),
    OP_DEFAULT(FLOAT_NOTEQUAL, STRVEC("a", "a"), "1"),
    OP_DEFAULT(FLOAT_LESS, STRVEC("a", "a"), "1"),
    OP_DEFAULT(FLOAT_LESSEQUAL, STRVEC("a", "a"), "1"),
    OP_DEFAULT(FLOAT_NAN, STRVEC("_"), "1"),
    OP_DEFAULT(FLOAT_ADD, STRVEC("a", "a"), "a"),
    OP_DEFAULT(FLOAT_SUB, STRVEC("a", "a"), "a"),
    OP_DEFAULT(FLOAT_MULT, STRVEC("a", "a"), "a"),
    OP_DEFAULT(FLOAT_DIV, STRVEC("a", "a"), "a"),
    OP_DEFAULT(FLOAT_NEG, STRVEC("a"), "a"),
    OP_DEFAULT(FLOAT_ABS, STRVEC("a"), "a"),
    OP_DEFAULT(FLOAT_SQRT, STRVEC("a"), "a"),
    OP_DEFAULT(FLOAT_CEIL, STRVEC("a"), "a"),
    OP_DEFAULT(FLOAT_FLOOR, STRVEC("a"), "a"),
    OP_DEFAULT(FLOAT_ROUND, STRVEC("a"), "a"),
    OP_CUSTOM_OPCODE(INT2FLOAT, STRVEC("_"), "_", ghidra::OpCode::CPUI_FLOAT_INT2FLOAT),
    OP_CUSTOM_OPCODE(FLOAT2FLOAT, STRVEC("_"), "_", ghidra::OpCode::CPUI_FLOAT_FLOAT2FLOAT),
    OP_CUSTOM_OPCODE(TRUNC, STRVEC("_"), "_", ghidra::OpCode::CPUI_FLOAT_TRUNC),

    OP_NO_OUT_OR(CPOOLREF, "*", "_"),
    OP_DEFAULT(NEW, STRVEC("_"), "_"),
};
Errorable<OpType> OpTypeFactory::getOpType(OpTypeToken token) {
  auto res = name2tp.find(token);
  if (res != name2tp.end()) {
    return alphaUpdate(res->second);
  } else {
    return err("Predefined op not found");
  }
}

OpType OpTypeFactory::alphaUpdate(const OpType &old) {
  counter++;

  return OpType{
      .opName = old.opName,
      .scheme = alphaUpdate(old.scheme),
      .ghidraOpCode = old.ghidraOpCode
  };
};

OpTypeScheme OpTypeFactory::alphaUpdate(const OpTypeScheme &old) {
  return OpTypeScheme{
      .inVarnodeConds = alphaUpdate(old.inVarnodeConds),
      .outVarnodeCond = alphaUpdate(old.outVarnodeCond),
      .isMultiequal = old.isMultiequal
  };
};

InVarnodeConditions OpTypeFactory::alphaUpdate(const InVarnodeConditions &old) {
  return std::visit(
      util::overloaded{
          [this](const InVarnodeConditionsArray &cond) -> InVarnodeConditions {
            return alphaUpdate(cond);
          },
          [](const InVarnodeConditionsSpecial &cond) -> InVarnodeConditions {
            return InVarnodeConditionsSpecial{};
          },
      },
      old
  );
};

InVarnodeConditionsArray
OpTypeFactory::alphaUpdate(const InVarnodeConditionsArray &old) {
  return InVarnodeConditionsArray{
      .array = map_vector(
          old.array,
          [this](const InVarnodeCondition &elem) -> InVarnodeCondition {
            return this->alphaUpdate(elem);
          }
      )
  };
};

InVarnodeCondition OpTypeFactory::alphaUpdate(const InVarnodeCondition &old) {
  return InVarnodeCondition{.size = alphaUpdate(old.size)};
};

Size OpTypeFactory::alphaUpdate(const Size &old) {
  return std::visit(
      util::overloaded{
          [this](const Id &id) -> Size { return alphaUpdate(id); },
          [](const ghidra::int4 &sz) -> Size { return sz; },
      },
      old
  );
};

Id OpTypeFactory::alphaUpdate(const Id &old) {
  return sizeVarFactory.createId(genNewName(old.getName()), false);
};

OutVarnodeCondition OpTypeFactory::alphaUpdate(const OutVarnodeCondition &old) {
  return std::visit(
      util::overloaded{
          [this](const OutVarnodeConditionDefault &cond)
              -> OutVarnodeCondition { return alphaUpdate(cond); },
          [this](const OutVarnodeConditionNoOutOr &cond)
              -> OutVarnodeCondition {
            return OutVarnodeConditionNoOutOr{.alt = alphaUpdate(cond.alt)};
          },
          [](const OutVarnodeConditionNoOut &) -> OutVarnodeCondition {
            return OutVarnodeConditionNoOut{};
          },
      },
      old
  );
};

OutVarnodeConditionDefault
OpTypeFactory::alphaUpdate(const OutVarnodeConditionDefault &old) {
  return OutVarnodeConditionDefault{.size = alphaUpdate(old.size)};
};
