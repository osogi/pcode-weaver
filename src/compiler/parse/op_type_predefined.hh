// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include "common/error.hh"
#include "parse/ast.hh"

#include <unordered_map>

namespace yy {
class Driver; // forward declaration
}

using OpTypeToken = int; // yy::Parser::token_kind_type;

class OpTypeFactory {
public:
  OpTypeFactory(
      std::unordered_map<OpTypeToken, ast::OpType> _name2tp,
      ast::IdFactory &_sizeVarFactory
  )
      : name2tp(_name2tp), sizeVarFactory(_sizeVarFactory) {};
  Errorable<ast::OpType> getOpType(OpTypeToken token);
  ast::OpType alphaUpdate(const ast::OpType &old);

private:
  std::unordered_map<OpTypeToken, ast::OpType> name2tp;
  ast::IdFactory &sizeVarFactory;
  uint32_t counter = 0;

  std::string genNewName(std::string oldName) {
    return "_pts" + std::to_string(counter) + oldName;
  }

  ast::OpTypeScheme alphaUpdate(const ast::OpTypeScheme &old);
  ast::InVarnodeConditions alphaUpdate(const ast::InVarnodeConditions &old);
  ast::InVarnodeConditionsArray
  alphaUpdate(const ast::InVarnodeConditionsArray &old);
  ast::InVarnodeCondition alphaUpdate(const ast::InVarnodeCondition &old);
  ast::Size alphaUpdate(const ast::Size &old);
  ast::Id alphaUpdate(const ast::Id &old);
  ast::OutVarnodeCondition alphaUpdate(const ast::OutVarnodeCondition &old);
  ast::OutVarnodeConditionDefault
  alphaUpdate(const ast::OutVarnodeConditionDefault &old);
};

extern std::unordered_map<OpTypeToken, ast::OpType> defaultOpType;
