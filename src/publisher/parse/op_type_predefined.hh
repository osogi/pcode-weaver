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
      yy::Driver &driver,
      std::unordered_map<OpTypeToken, ast::OpType> name2tp
  )
      : driver(driver), name2tp(name2tp) {};

  Errorable<ast::OpType> getOpType(OpTypeToken token);

private:
  yy::Driver &driver;
  std::unordered_map<OpTypeToken, ast::OpType> name2tp;
  uint32_t counter = 0;

  std::string genNewName(std::string oldName){
    return "_pts"  + std::to_string(counter) + oldName;
  }

  ast::OpType alphaUpdate(const ast::OpType& old);
  ast::OpTypeScheme alphaUpdate(const ast::OpTypeScheme& old);
  ast::InVarnodeConditions alphaUpdate(const ast::InVarnodeConditions& old);
  ast::InVarnodeConditionsArray alphaUpdate(const ast::InVarnodeConditionsArray& old);
  ast::InVarnodeCondition alphaUpdate(const ast::InVarnodeCondition& old);
  ast::Size alphaUpdate(const ast::Size& old);
  ast::Id alphaUpdate(const ast::Id& old);
  ast::OutVarnodeCondition alphaUpdate(const ast::OutVarnodeCondition& old);
  ast::OutVarnodeConditionDefault alphaUpdate(const ast::OutVarnodeConditionDefault& old);
};


extern std::unordered_map<OpTypeToken, ast::OpType> defaultOpType;
