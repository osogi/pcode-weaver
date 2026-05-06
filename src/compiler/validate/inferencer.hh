#pragma once

#include "validate/action_pcodegraph.hh"
#include "validate/pcodegraph.hh"
#include "validate/solvers.hh"
#include "validate/specconditions.hh"

namespace infer {
template <typename Term>
Errorable<void> errorNoSpec(const Term &a, const Term &b) {
  std::ostringstream s;
  s << "TypeVariable " << a
    << " don't have specialised value, instead it equal to " << b;
  return err(s.str());
}

class Inferencer {
  using CondVectType = std::vector<speccond::SpecCondition>;

protected:
  Context &cntx;
  solvers::SizeSolver &sizeSolver;
  solvers::BBSolver &bbSolver;

  struct SizeEqualPolicy {
    using Term = solvers::SizeTerm;
    using SpecValue = specvalues::SpecValueSize;
    using Cond = speccond::SizeEqual;
    using SolverType = solvers::SizeSolver;

    static bool check(SolverType &s, const Term &a, const Term &b) {
      return s.isEqual(a, b);
    }
    static Errorable<void>
    addToSolver(SolverType &s, const Term &a, const Term &b) {
      return s.addEquation(a, b).and_then([](bool) {
        return Errorable<void>{};
      });
    }
  };

  struct BBEqualPolicy {
    using Term = solvers::BBTerm;
    using SpecValue = specvalues::SpecValueBB;
    using Cond = speccond::BBEqual;
    using SolverType = solvers::BBSolver;

    static bool check(SolverType &s, const Term &a, const Term &b) {
      return s.isEqual(a, b);
    }
    static Errorable<void>
    addToSolver(SolverType &s, const Term &a, const Term &b) {
      return s.addEquation(a, b).and_then([](bool) {
        return Errorable<void>{};
      });
    }
  };

  struct BBDominatePolicy {
    using Term = solvers::BBTerm;
    using SpecValue = specvalues::SpecValueBB;
    using Cond = speccond::BBDominate;
    using SolverType = solvers::BBSolver;

    static bool check(SolverType &s, const Term &a, const Term &b) {
      return s.isLessOrEqual(a, b);
    }
    static Errorable<void>
    addToSolver(SolverType &s, const Term &a, const Term &b) {
      return s.addLessOrEqual(a, b).and_then([](bool) {
        return Errorable<void>{};
      });
    }
  };

  // Generic algorithm (template definition)
  template <typename Policy>
  Errorable<void> addRelation(
      typename Policy::SolverType &solver, const typename Policy::Term &a,
      const typename Policy::Term &b, CondVectType *conds
  ) {
    if (conds && Policy::check(solver, a, b))
      return {};
    if (conds) {
      auto fa = solver.find(a);
      auto fb = solver.find(b);
      auto *sa = std::get_if<typename Policy::SpecValue>(&fa);
      auto *sb = std::get_if<typename Policy::SpecValue>(&fb);
      if (!sa)
        return errorNoSpec(a, fa);
      if (!sb)
        return errorNoSpec(b, fb);
      conds->push_back(typename Policy::Cond(*sa, *sb));
    }
    return Policy::addToSolver(solver, a, b);
  }
  Errorable<void> addSizeEqual(
      const solvers::SizeTerm &a, const solvers::SizeTerm &b,
      CondVectType *conds
  ) {
    return addRelation<SizeEqualPolicy>(sizeSolver, a, b, conds);
  }
  Errorable<void> addBBEqual(
      const solvers::BBTerm &a, const solvers::BBTerm &b, CondVectType *conds
  ) {
    return addRelation<BBEqualPolicy>(bbSolver, a, b, conds);
  }
  Errorable<void> addBBDominate(
      const solvers::BBTerm &a, const solvers::BBTerm &b, CondVectType *conds
  ) {
    return addRelation<BBDominatePolicy>(bbSolver, a, b, conds);
  }
  Errorable<specvalues::SpecValueBB> resolveBBSpec(const ast::Id &id);
  Errorable<void> addSizeEqualSizeAndVarnode(
      const ast::Size &sz, const graph::GraphVarnode *gvn, CondVectType *conds
  );
  Errorable<void>
  inferenceVarnode(const graph::GraphVarnode &gvn, CondVectType *conds);
  Errorable<void>
  inferencePnode(const graph::GraphPnode &gvn, CondVectType *conds);
  Errorable<void> inferenceVarnodeUserConds(
      const graph::GraphVarnode &gvn, CondVectType *conds
  );
  Errorable<void>
  inferencePnodeUserConds(const graph::GraphPnode &gvn, CondVectType *conds);

public:
  Inferencer(
      Context &_context, const graph::PcodeGraph &_graph,
      solvers::SizeSolver &_sizeSolver, solvers::BBSolver &_bbSolver
  )
      : cntx(_context), sizeSolver(_sizeSolver), bbSolver(_bbSolver) {}
  Errorable<void>
  inference(const graph::PcodeGraph &pgraph, CondVectType *conds = nullptr);
  Errorable<void> inference(
      const graph::ActionPcodeGraph &pgraph, CondVectType *conds = nullptr
  );
  Errorable<void> inferenceUserConds(
      const graph::PcodeGraph &pgraph, CondVectType *conds = nullptr
  );
};
} // namespace infer
