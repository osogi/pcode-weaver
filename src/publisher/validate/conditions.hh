#pragma once

#include "parse/ast.hh"

class Condition {
public:
  virtual std::string toString() const = 0;
};

// a <= b
class ConditionBBDominate : Condition {
private:
  ast::BasicBlockVar a;
  ast::BasicBlockVar b;

public:
  ConditionBBDominate(
      const ast::BasicBlockVar &first, const ast::BasicBlockVar &second
  )
      : a(first), b(second) {};

  std::string toString() const {
    return a.id.getName() + " <= " + b.id.getName();
  }
};