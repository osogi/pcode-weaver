#include "parse/op_type_predefined.hh"

// generated headers
#include "parse/parser.hh"

#include <cassert>
#include <unordered_map>

using namespace ast;

static IdFactory helpIdFactory("_predef_op_tp_");

Size createSize(const std::string &varName) {
  if (varName == "_") {
    return helpIdFactory.createId();
  }
  return helpIdFactory.createId(varName);
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

#define OP(token, inConds, outCond)                                            \
  (std::pair<OpTypeToken, OpTypePredefined>{                   \
      yy::Parser::token_kind_type::RULES_TOKEN_##token,                        \
      OpTypePredefined{#token, OpTypeGeneral{inConds, outCond, false}}         \
  })

#define OP_CUSTOM_OUT(name, inCondsRaw, outCond)                               \
  OP(name, createInConds(inCondsRaw), outCond)

#define OP_DEFAULT(name, inCondsRaw, outCondRaw)                               \
  OP_CUSTOM_OUT(                                                               \
      name, inCondsRaw, OutVarnodeConditionDefault{createSize(outCondRaw)}     \
  )

#define STRVEC(...) (std::vector<std::string>{__VA_ARGS__})

std::unordered_map<OpTypeToken, OpTypePredefined>
    defaultOpTypePredefined = {
        OP_DEFAULT(INT_ADD, STRVEC("a", "a"), "a"),
        OP_DEFAULT(INT_SUB, STRVEC("a", "a"), "a"),
};

Errorable<OpTypePredefined> OpTypePredefinedFactory::getOpTypePredefined(
    OpTypeToken token
) {
  counter++;
  auto res = name2tp.find(token);
  if (res != name2tp.end()) {
    return alphaUpdate(res->second);
  } else {
    return err("Predefined op not found");
  }
}

OpTypePredefined
OpTypePredefinedFactory::alphaUpdate(const OpTypePredefined &old) {
  return OpTypePredefined{
      .opName = old.opName, .generalType = alphaUpdate(old.generalType)
  };
};

OpTypeGeneral OpTypePredefinedFactory::alphaUpdate(const OpTypeGeneral &old) {
  return OpTypeGeneral{
      .inVarnodeConds = alphaUpdate(old.inVarnodeConds),
      .outVarnodeCond = alphaUpdate(old.outVarnodeCond),
      .isMultiequal = old.isMultiequal
  };
};

InVarnodeConditions
OpTypePredefinedFactory::alphaUpdate(const InVarnodeConditions &old) {
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
OpTypePredefinedFactory::alphaUpdate(const InVarnodeConditionsArray &old) {
  return InVarnodeConditionsArray{
      .array = map_vector(
          old.array,
          [this](const InVarnodeCondition &elem) -> InVarnodeCondition {
            return this->alphaUpdate(elem);
          }
      )
  };
};

InVarnodeCondition
OpTypePredefinedFactory::alphaUpdate(const InVarnodeCondition &old) {
  return InVarnodeCondition{.size = alphaUpdate(old.size)};
};

Size OpTypePredefinedFactory::alphaUpdate(const Size &old) {
  return std::visit(
      util::overloaded{
          [this](const Id &id) -> Size { return alphaUpdate(id); },
          [](const ghidra::int4 &sz) -> Size { return sz; },
      },
      old
  );
};

Id OpTypePredefinedFactory::alphaUpdate(const Id &old) {
  return helpIdFactory.createId(genNewName(old.getName()));
};

OutVarnodeCondition
OpTypePredefinedFactory::alphaUpdate(const OutVarnodeCondition &old) {
  return std::visit(
      util::overloaded{
          [this](const OutVarnodeConditionDefault &cond) -> OutVarnodeCondition {
            return alphaUpdate(cond);
          },
          [this](const OutVarnodeConditionNoOutOr &cond) -> OutVarnodeCondition {
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
OpTypePredefinedFactory::alphaUpdate(const OutVarnodeConditionDefault &old) {
  return OutVarnodeConditionDefault{.size = alphaUpdate(old.size)};
};
