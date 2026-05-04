#include "pwrule.hh"

#include <ghidra/block.hh>
#include <ghidra/op.hh>
#include <ghidra/varnode.hh>

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

using pcodeweaver::compiled::Check;
using pcodeweaver::compiled::CheckKind;
using pcodeweaver::compiled::CheckValue;
using pcodeweaver::compiled::CheckValueKind;
using pcodeweaver::compiled::EdgeCheck;
using pcodeweaver::compiled::MatchStep;
using pcodeweaver::compiled::PatternProgram;
using pcodeweaver::compiled::SourceKind;
using pcodeweaver::compiled::StepId;
using pcodeweaver::compiled::StepKind;
using pcodeweaver::compiled::StepSource;

enum class CandidateKind {
  Empty,
  Pnode,
  Varnode,
};

struct Candidate {
  CandidateKind kind = CandidateKind::Empty;
  ghidra::PcodeOp *op = nullptr;
  ghidra::Varnode *vn = nullptr;
};

struct MatchState {
  std::unordered_map<StepId, ghidra::PcodeOp *> pnodes;
  std::unordered_map<StepId, ghidra::Varnode *> varnodes;
  std::unordered_set<StepId> emptySteps;
  std::unordered_set<ghidra::PcodeOp *> usedPnodes;
  std::unordered_set<ghidra::Varnode *> usedVarnodes;
};

struct CheckScalar {
  bool valid = false;
  std::int64_t value = 0;
};

struct CheckBlock {
  bool valid = false;
  const ghidra::FlowBlock *block = nullptr;
};

Candidate emptyCandidate() { return {}; }

Candidate pnodeCandidate(ghidra::PcodeOp *op) {
  Candidate candidate;
  candidate.kind = CandidateKind::Pnode;
  candidate.op = op;
  return candidate;
}

Candidate varnodeCandidate(ghidra::Varnode *vn) {
  Candidate candidate;
  candidate.kind = CandidateKind::Varnode;
  candidate.vn = vn;
  return candidate;
}

bool isRealCandidate(const Candidate &candidate) {
  return (candidate.kind == CandidateKind::Pnode && candidate.op != nullptr) ||
         (candidate.kind == CandidateKind::Varnode && candidate.vn != nullptr);
}

bool sameCandidate(const Candidate &left, const Candidate &right) {
  if (left.kind != right.kind) {
    return false;
  }
  if (left.kind == CandidateKind::Pnode) {
    return left.op == right.op;
  }
  if (left.kind == CandidateKind::Varnode) {
    return left.vn == right.vn;
  }
  return true;
}

ghidra::PcodeOp *getPnode(const MatchState &state, StepId step) {
  const auto iter = state.pnodes.find(step);
  return iter == state.pnodes.end() ? nullptr : iter->second;
}

ghidra::Varnode *getVarnode(const MatchState &state, StepId step) {
  const auto iter = state.varnodes.find(step);
  return iter == state.varnodes.end() ? nullptr : iter->second;
}

void clearStep(MatchState &state, StepId step) {
  const auto pnodeIter = state.pnodes.find(step);
  if (pnodeIter != state.pnodes.end()) {
    state.usedPnodes.erase(pnodeIter->second);
    state.pnodes.erase(pnodeIter);
  }

  const auto varnodeIter = state.varnodes.find(step);
  if (varnodeIter != state.varnodes.end()) {
    state.usedVarnodes.erase(varnodeIter->second);
    state.varnodes.erase(varnodeIter);
  }

  state.emptySteps.erase(step);
}

void bindStep(MatchState &state, StepId step, const Candidate &candidate) {
  clearStep(state, step);
  if (candidate.kind == CandidateKind::Pnode) {
    state.pnodes.emplace(step, candidate.op);
    state.usedPnodes.insert(candidate.op);
  } else if (candidate.kind == CandidateKind::Varnode) {
    state.varnodes.emplace(step, candidate.vn);
    state.usedVarnodes.insert(candidate.vn);
  } else {
    state.emptySteps.insert(step);
  }
}

bool getExpectedCandidate(
    const MatchState &state, StepId step, Candidate &candidate
) {
  ghidra::PcodeOp *op = getPnode(state, step);
  if (op != nullptr) {
    candidate = pnodeCandidate(op);
    return true;
  }

  ghidra::Varnode *vn = getVarnode(state, step);
  if (vn != nullptr) {
    candidate = varnodeCandidate(vn);
    return true;
  }

  if (state.emptySteps.find(step) != state.emptySteps.end()) {
    candidate = emptyCandidate();
    return true;
  }
  return false;
}

bool alreadyBound(const MatchState &state, const Candidate &candidate) {
  if (!isRealCandidate(candidate)) {
    return false;
  }

  if (candidate.kind == CandidateKind::Pnode) {
    return state.usedPnodes.find(candidate.op) != state.usedPnodes.end();
  }
  if (candidate.kind == CandidateKind::Varnode) {
    return state.usedVarnodes.find(candidate.vn) != state.usedVarnodes.end();
  }
  return false;
}

MatchState makeMatchState(std::size_t stepCount) {
  MatchState state;
  state.pnodes.reserve(stepCount);
  state.varnodes.reserve(stepCount);
  state.emptySteps.reserve(stepCount);
  state.usedPnodes.reserve(stepCount);
  state.usedVarnodes.reserve(stepCount);
  return state;
}

template <class Visitor>
bool visitSourceCandidates(
    const StepSource &source, const MatchState &state, ghidra::PcodeOp *root,
    Visitor visitor
) {
  switch (source.kind) {
  case SourceKind::RootPnode:
    return visitor(pnodeCandidate(root));

  case SourceKind::PnodeInput: {
    ghidra::PcodeOp *from = getPnode(state, source.from);
    if (from == nullptr ||
        source.inputIndex >= static_cast<std::uint32_t>(from->numInput())) {
      return false;
    }
    return visitor(varnodeCandidate(
        from->getIn(static_cast<ghidra::int4>(source.inputIndex))
    ));
  }

  case SourceKind::PnodeOutput: {
    ghidra::PcodeOp *from = getPnode(state, source.from);
    if (from == nullptr) {
      return false;
    }
    ghidra::Varnode *out = from->getOut();
    return visitor(out == nullptr ? emptyCandidate() : varnodeCandidate(out));
  }

  case SourceKind::VarnodeDef: {
    ghidra::Varnode *from = getVarnode(state, source.from);
    if (from == nullptr) {
      return false;
    }
    ghidra::PcodeOp *def = from->getDef();
    return visitor(def == nullptr ? emptyCandidate() : pnodeCandidate(def));
  }

  case SourceKind::VarnodeDescend: {
    ghidra::Varnode *vn = getVarnode(state, source.from);
    if (vn == nullptr) {
      return false;
    }

    for (auto iter = vn->beginDescend(); iter != vn->endDescend(); ++iter) {
      ghidra::PcodeOp *descend = *iter;
      if (descend == nullptr ||
          source.inputIndex >=
              static_cast<std::uint32_t>(descend->numInput())) {
        continue;
      }
      if (descend->getIn(static_cast<ghidra::int4>(source.inputIndex)) == vn) {
        if (visitor(pnodeCandidate(descend))) {
          return true;
        }
      }
    }
    return false;
  }
  }

  return false;
}

bool stepKindMatches(const MatchStep &step, const Candidate &candidate) {
  switch (step.kind) {
  case StepKind::AnyVarnode:
    return candidate.kind == CandidateKind::Varnode && candidate.vn != nullptr;

  case StepKind::Constant:
    return candidate.kind == CandidateKind::Varnode &&
           candidate.vn != nullptr && candidate.vn->isConstant() &&
           static_cast<std::uint64_t>(candidate.vn->getOffset()) ==
               static_cast<std::uint64_t>(step.constant);

  case StepKind::Empty:
    return !isRealCandidate(candidate);

  case StepKind::Pnode:
    if (candidate.kind != CandidateKind::Pnode || candidate.op == nullptr ||
        candidate.op->isDead()) {
      return false;
    }
    if (step.hasOpCode &&
        candidate.op->code() != static_cast<ghidra::OpCode>(step.opCode)) {
      return false;
    }
    return true;
  }

  return false;
}

bool edgeCheckMatches(
    const EdgeCheck &check, const MatchState &state, ghidra::PcodeOp *root
) {
  Candidate expected;
  if (!getExpectedCandidate(state, check.expected, expected)) {
    return false;
  }

  return visitSourceCandidates(
      check.source, state, root, [&](const Candidate &candidate) {
        return sameCandidate(candidate, expected);
      }
  );
}

CheckScalar evalScalar(const CheckValue &value, const MatchState &state) {
  switch (value.kind) {
  case CheckValueKind::ConstantSize:
    return {true, value.constant};

  case CheckValueKind::VarnodeSize: {
    ghidra::Varnode *vn = getVarnode(state, value.step);
    if (vn == nullptr) {
      return {};
    }
    return {true, vn->getSize()};
  }

  case CheckValueKind::VarnodeBasicBlock:
  case CheckValueKind::PnodeBasicBlock:
    return {};
  }

  return {};
}

CheckBlock evalBlock(const CheckValue &value, const MatchState &state) {
  switch (value.kind) {
  case CheckValueKind::PnodeBasicBlock: {
    ghidra::PcodeOp *op = getPnode(state, value.step);
    if (op == nullptr) {
      return {};
    }
    return {true, op->getParent()};
  }

  case CheckValueKind::VarnodeBasicBlock: {
    ghidra::Varnode *vn = getVarnode(state, value.step);
    if (vn == nullptr || vn->getDef() == nullptr) {
      return {};
    }
    return {true, vn->getDef()->getParent()};
  }

  case CheckValueKind::ConstantSize:
  case CheckValueKind::VarnodeSize:
    return {};
  }

  return {};
}

bool checkMatches(const Check &check, const MatchState &state) {
  switch (check.kind) {
  case CheckKind::SizeEqual: {
    const CheckScalar left = evalScalar(check.left, state);
    const CheckScalar right = evalScalar(check.right, state);
    return left.valid && right.valid && left.value == right.value;
  }

  case CheckKind::BasicBlockEqual: {
    const CheckBlock left = evalBlock(check.left, state);
    const CheckBlock right = evalBlock(check.right, state);
    return left.valid && right.valid && left.block != nullptr &&
           left.block == right.block;
  }

  case CheckKind::BasicBlockDominates: {
    const CheckBlock left = evalBlock(check.left, state);
    const CheckBlock right = evalBlock(check.right, state);
    return left.valid && right.valid && left.block != nullptr &&
           right.block != nullptr && left.block->dominates(right.block);
  }
  }

  return false;
}

bool postBindChecksMatch(
    const MatchStep &step, const MatchState &state, ghidra::PcodeOp *root
) {
  for (const EdgeCheck &edgeCheck : step.edgeChecks) {
    if (!edgeCheckMatches(edgeCheck, state, root)) {
      return false;
    }
  }
  for (const Check &check : step.checks) {
    if (!checkMatches(check, state)) {
      return false;
    }
  }
  return true;
}

bool matchFromStep(
    const PatternProgram &program, ghidra::PcodeOp *root, StepId stepId,
    MatchState &state
) {
  if (static_cast<std::size_t>(stepId) >= program.steps.size()) {
    return true;
  }

  const MatchStep &step = program.steps[static_cast<std::size_t>(stepId)];
  return visitSourceCandidates(
      step.source, state, root, [&](const Candidate &candidate) {
        if (alreadyBound(state, candidate) ||
            !stepKindMatches(step, candidate)) {
          return false;
        }

        bindStep(state, stepId, candidate);
        if (postBindChecksMatch(step, state, root) &&
            matchFromStep(program, root, stepId + 1, state)) {
          return true;
        }
        clearStep(state, stepId);
        return false;
      }
  );
}

} // namespace

ghidra::int4 PcodeWeaverRule::applyPatternToPnode(
    ghidra::Funcdata &data, ghidra::PcodeOp *op
) {
  (void)data;

  if (op == nullptr || op->isDead()) {
    return 0;
  }

  MatchState state = makeMatchState(compiled.pattern.steps.size());
  if (!matchFromStep(compiled.pattern, op, 0, state)) {
    return 0;
  }

  patternPnodes = std::move(state.pnodes);
  patternVarnodes = std::move(state.varnodes);
  patternEmptySteps = std::move(state.emptySteps);
  return 1;
}

ghidra::int4 PcodeWeaverRule::applyPattern(ghidra::Funcdata &data) {
  patternPnodes.clear();
  patternVarnodes.clear();
  patternEmptySteps.clear();

  if (compiled.pattern.steps.empty()) {
    return 0;
  }

  const MatchStep &root = compiled.pattern.steps.front();
  if (root.kind != StepKind::Pnode) {
    return 0;
  }

  if (!root.hasOpCode) {
    return 0;
  }

  const auto opCode = static_cast<ghidra::OpCode>(root.opCode);
  for (auto iter = data.beginOp(opCode); iter != data.endOp(opCode); ++iter) {
    if (applyPatternToPnode(data, *iter) != 0) {
      return 1;
    }
  }
  return 0;
}

ghidra::int4 PcodeWeaverRule::apply(ghidra::Funcdata &data) {
  return applyPattern(data);
}
