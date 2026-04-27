#pragma once

#include "parse/ast.hh"
#include "parse/ast_print.hh"
#include "validate/specvalues.hh"

#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace solvers {

template <typename TermType> class EqualitySolver {
protected:
  std::unordered_map<TermType, TermType> parent;

  virtual Errorable<void> unite(const TermType &x, const TermType &y) = 0;

public:
  virtual ~EqualitySolver() = default;

  bool contains(const TermType &x) { return parent.contains(x); }

  TermType find(const TermType &x) {
    // Initialize if not exists
    if (!contains(x)) {
      parent[x] = x;
    }

    // Path compression
    if (parent[x] != x) {
      parent[x] = find(parent[x]);
    }

    return parent[x];
  }

  std::optional<TermType> findopt(const TermType &x) {
    if (contains(x)) {
      return find(x);
    } else {
      return std::nullopt;
    }
  }

  /// @brief  Add equation: term1 = term2
  /// @param left
  /// @param right
  /// @return false if nothing change; true if new equality add; error on error
  Errorable<bool> addEquation(const TermType &left, const TermType &right) {
    // if already equal return false
    if (isEqual(left, right)) {
      return false;
    } else {
      // if there isn't coflicts return true
      auto unite_res = unite(left, right);
      if (unite_res.has_value()) {
        return true;
      } else { // error
        return std::unexpected(unite_res.error());
      }
    }
  }

  bool isEqual(const TermType &x, const TermType &y) {
    return find(x) == find(y);
  }
};

using Id = ast::Id;
using SizeValue = specvalues::SpecValueSize;
using SizeTerm = std::variant<SizeValue, Id>;

class SizeSolver : public EqualitySolver<SizeTerm> {
protected:
  Errorable<void> unite(const SizeTerm &x, const SizeTerm &y) override;
};

using BBValue = specvalues::SpecValueBB;
using BBTerm = std::variant<BBValue, Id>;

class BBSolver : public EqualitySolver<BBTerm> {
protected:
  std::vector<std::pair<BBTerm, BBTerm>> inequalities;
  std::unordered_map<BBTerm, std::vector<BBTerm>> dag;
  bool dirty = true;

  void rebuild();

  bool reachable(BBTerm from, BBTerm to);
  Errorable<void> unite(const BBTerm &x, const BBTerm &y) override;

public:
  bool addLessOrEqual(BBTerm a, BBTerm b);
  bool isLessOrEqual(BBTerm a, BBTerm b);

  BBTerm find(const BBTerm &x);
  bool isEqual(const BBTerm &x, const BBTerm &y);
  bool contains(const BBTerm &x);
  ;
};

std::ostream &operator<<(std::ostream &os, const SizeTerm &term);
std::ostream &operator<<(std::ostream &os, const BBTerm &term);

} // namespace solvers

using solvers::operator<<;