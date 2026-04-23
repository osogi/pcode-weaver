#pragma once

#include "parse/ast.hh"
#include "parse/ast_print.hh"

#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace solvers {

using Id = ast::Id;
using Value = int64_t;
using Term = std::variant<Value, Id>;

class EqualitySolver {
private:
  std::unordered_map<Term, Term> parent;

  bool unite(const Term &x, const Term &y);

public:
  Term find(const Term &x);

  std::optional<Term> findopt(const Term &x);

  // Add equation: term1 = term2
  Errorable<bool> addEquation(const Term &left, const Term &right);

  // Check if two terms are equal
  bool isEqual(const Term &x, const Term &y);
};

class PartialOrderSolver {
private:
  std::vector<std::pair<Id, Id>> inequalities;
  std::unordered_map<Id, Id> parent; // ID -> smallest representative
  std::unordered_map<Id, std::vector<Id>> dag;
  bool dirty = true;

  void rebuild();
  
  bool reachable(Id from, Id to);
  
  public:
  Id find(Id x);
  std::optional<Term> findopt(const Id &x);

  bool addLessOrEqual(Id a, Id b);
  bool isEqual(Id a, Id b);
  bool isLessOrEqual(Id a, Id b);
};

} // namespace solvers

std::ostream &operator<<(std::ostream &os, const solvers::Term &term);