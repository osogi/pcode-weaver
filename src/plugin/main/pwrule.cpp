#include "pwrule.hh"

#include <ghidra/block.hh>
#include <ghidra/op.hh>
#include <ghidra/varnode.hh>

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

using pcodeweaver::compiled::ActionNodeKind;
using pcodeweaver::compiled::ActionNodeRef;
using pcodeweaver::compiled::ActionStep;
using pcodeweaver::compiled::ActionStepKind;
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
  if (!isRealCandidate(left) && !isRealCandidate(right)) {
    return true;
  }

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
  if (!isRealCandidate(candidate)) {
    state.emptySteps.insert(step);
  } else if (candidate.kind == CandidateKind::Pnode) {
    state.pnodes.emplace(step, candidate.op);
    state.usedPnodes.insert(candidate.op);
  } else {
    state.varnodes.emplace(step, candidate.vn);
    state.usedVarnodes.insert(candidate.vn);
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
    if (root == nullptr) {
      return false;
    }
    return visitor(pnodeCandidate(root));

  case SourceKind::PnodeInput: {
    ghidra::PcodeOp *from = getPnode(state, source.from);
    if (from == nullptr) {
      return false;
    }
    if (source.inputIndex >= static_cast<std::uint32_t>(from->numInput())) {
      return visitor(emptyCandidate());
    }
    ghidra::Varnode *in =
        from->getIn(static_cast<ghidra::int4>(source.inputIndex));
    if (in == nullptr) {
      return false;
    }
    return visitor(varnodeCandidate(in));
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

    bool hasCandidate = false;
    for (auto iter = vn->beginDescend(); iter != vn->endDescend(); ++iter) {
      ghidra::PcodeOp *descend = *iter;
      if (descend == nullptr ||
          source.inputIndex >=
              static_cast<std::uint32_t>(descend->numInput())) {
        continue;
      }
      if (descend->getIn(static_cast<ghidra::int4>(source.inputIndex)) == vn) {
        hasCandidate = true;
        if (visitor(pnodeCandidate(descend))) {
          return true;
        }
      }
    }
    return hasCandidate ? false : visitor(emptyCandidate());
  }
  }

  return false;
}

bool stepKindMatches(const MatchStep &step, const Candidate &candidate) {
  switch (step.kind) {
  case StepKind::AnyVarnode:
    return candidate.kind == CandidateKind::Varnode &&
           candidate.vn != nullptr;

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
  case CheckValueKind::ConstantOffset:
    return {true, value.constant};

  case CheckValueKind::VarnodeSize: {
    ghidra::Varnode *vn = getVarnode(state, value.step);
    if (vn == nullptr) {
      return {};
    }
    return {true, vn->getSize()};
  }

  case CheckValueKind::VarnodeOffset: {
    ghidra::Varnode *vn = getVarnode(state, value.step);
    if (vn == nullptr) {
      return {};
    }
    return {true, static_cast<std::int64_t>(vn->getOffset())};
  }

  case CheckValueKind::VarnodeBasicBlock:
  case CheckValueKind::PnodeBasicBlock:
    return {};
  }

  return {};
}

CheckScalar evalScalar(
    const CheckValue &value,
    const std::unordered_map<StepId, ghidra::Varnode *> &varnodes
) {
  switch (value.kind) {
  case CheckValueKind::ConstantSize:
  case CheckValueKind::ConstantOffset:
    return {true, value.constant};

  case CheckValueKind::VarnodeSize: {
    auto iter = varnodes.find(value.step);
    if (iter == varnodes.end() || iter->second == nullptr) {
      return {};
    }
    return {true, iter->second->getSize()};
  }

  case CheckValueKind::VarnodeOffset: {
    auto iter = varnodes.find(value.step);
    if (iter == varnodes.end() || iter->second == nullptr) {
      return {};
    }
    return {true, static_cast<std::int64_t>(iter->second->getOffset())};
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
  case CheckValueKind::ConstantOffset:
  case CheckValueKind::VarnodeSize:
  case CheckValueKind::VarnodeOffset:
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

  case CheckKind::OffsetEqual: {
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

ghidra::PcodeOp *lookupActionPnode(
    const ActionNodeRef &ref,
    const std::unordered_map<StepId, ghidra::PcodeOp *> &pnodes
) {
  if (ref.kind != ActionNodeKind::Pnode) {
    return nullptr;
  }
  auto iter = pnodes.find(ref.step);
  return iter == pnodes.end() ? nullptr : iter->second;
}

ghidra::Varnode *lookupActionVarnode(
    const ActionNodeRef &ref,
    const std::unordered_map<StepId, ghidra::Varnode *> &varnodes
) {
  if (ref.kind != ActionNodeKind::Varnode) {
    return nullptr;
  }
  auto iter = varnodes.find(ref.step);
  return iter == varnodes.end() ? nullptr : iter->second;
}

ghidra::int4 fallbackInputSize(ghidra::PcodeOp *op, std::uint32_t inputIndex) {
  if (op != nullptr &&
      inputIndex < static_cast<std::uint32_t>(op->numInput())) {
    ghidra::Varnode *current = op->getIn(static_cast<ghidra::int4>(inputIndex));
    if (current != nullptr) {
      return current->getSize();
    }
  }
  return 1;
}

ghidra::Varnode *resolveInputVarnode(
    ghidra::Funcdata &data, const ActionNodeRef &ref, ghidra::PcodeOp *target,
    std::uint32_t inputIndex,
    const std::unordered_map<StepId, ghidra::Varnode *> &varnodes
) {
  switch (ref.kind) {
  case ActionNodeKind::Varnode:
    return lookupActionVarnode(ref, varnodes);

  case ActionNodeKind::Constant:
    return data.newConstant(
        fallbackInputSize(target, inputIndex),
        static_cast<ghidra::uintb>(ref.constant)
    );

  case ActionNodeKind::Empty:
  case ActionNodeKind::Pnode:
    return nullptr;
  }

  return nullptr;
}

bool setPnodeInput(
    ghidra::Funcdata &data, ghidra::PcodeOp *op, ghidra::Varnode *vn,
    std::uint32_t inputIndex
) {
  if (op == nullptr || op->isDead()) {
    return false;
  }

  ghidra::int4 slot = static_cast<ghidra::int4>(inputIndex);
  if (vn == nullptr) {
    if (inputIndex >= static_cast<std::uint32_t>(op->numInput())) {
      return false;
    }
    data.opUnsetInput(op, slot);
    return true;
  }

  if (inputIndex < static_cast<std::uint32_t>(op->numInput())) {
    data.opSetInput(op, vn, slot);
    return true;
  }
  if (inputIndex == static_cast<std::uint32_t>(op->numInput())) {
    data.opInsertInput(op, vn, slot);
    return true;
  }
  return false;
}

bool setPnodeOutput(
    ghidra::Funcdata &data, ghidra::PcodeOp *op, ghidra::Varnode *vn
) {
  if (op == nullptr || op->isDead()) {
    return false;
  }
  if (vn == nullptr) {
    if (op->getOut() != nullptr) {
      data.opUnsetOutput(op);
    }
    return true;
  }

  ghidra::PcodeOp *oldDef = vn->getDef();
  if (oldDef != nullptr && oldDef != op && oldDef->getOut() == vn) {
    data.opUnsetOutput(oldDef);
  }
  if (op->getOut() != nullptr && op->getOut() != vn) {
    data.opUnsetOutput(op);
  }
  data.opSetOutput(op, vn);
  return true;
}

bool applyActionStep(
    ghidra::Funcdata &data, const ActionStep &step,
    std::unordered_map<StepId, ghidra::PcodeOp *> &pnodes,
    std::unordered_map<StepId, ghidra::Varnode *> &varnodes
) {
  switch (step.kind) {
  case ActionStepKind::CreateVarnode: {
    if (step.target.kind != ActionNodeKind::Varnode || !step.hasSize) {
      return false;
    }
    CheckScalar size = evalScalar(step.size, varnodes);
    if (!size.valid || size.value <= 0) {
      return false;
    }
    ghidra::Varnode *vn = data.newUnique(static_cast<ghidra::int4>(size.value));
    if (vn == nullptr) {
      return false;
    }
    varnodes[step.target.step] = vn;
    return true;
  }

  case ActionStepKind::CreatePnode: {
    if (step.target.kind != ActionNodeKind::Pnode || !step.hasOpCode) {
      return false;
    }
    ghidra::PcodeOp *anchor = lookupActionPnode(step.value, pnodes);
    if (anchor == nullptr || anchor->isDead()) {
      return false;
    }

    ghidra::PcodeOp *op = data.newOp(
        static_cast<ghidra::int4>(step.inputCount), anchor->getAddr()
    );
    if (op == nullptr) {
      return false;
    }
    data.opSetOpcode(op, static_cast<ghidra::OpCode>(step.opCode));
    if (step.insertBefore) {
      data.opInsertBefore(op, anchor);
    } else {
      data.opInsertAfter(op, anchor);
    }
    pnodes[step.target.step] = op;
    return true;
  }

  case ActionStepKind::SetPnodeOutput: {
    ghidra::PcodeOp *op = lookupActionPnode(step.target, pnodes);
    ghidra::Varnode *vn = nullptr;
    if (step.value.kind == ActionNodeKind::Varnode) {
      vn = lookupActionVarnode(step.value, varnodes);
      if (vn == nullptr) {
        return false;
      }
    } else if (step.value.kind != ActionNodeKind::Empty) {
      return false;
    }
    return setPnodeOutput(data, op, vn);
  }

  case ActionStepKind::SetPnodeInput: {
    ghidra::PcodeOp *op = lookupActionPnode(step.target, pnodes);
    ghidra::Varnode *vn =
        resolveInputVarnode(data, step.value, op, step.inputIndex, varnodes);
    if (step.value.kind != ActionNodeKind::Empty && vn == nullptr) {
      return false;
    }
    return setPnodeInput(data, op, vn, step.inputIndex);
  }

  case ActionStepKind::DeletePnode: {
    ghidra::PcodeOp *op = lookupActionPnode(step.target, pnodes);
    if (op == nullptr || op->isDead()) {
      return false;
    }
    data.opUnlink(op);
    data.opDestroy(op);
    pnodes.erase(step.target.step);
    return true;
  }
  }

  return false;
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

ghidra::int4 PcodeWeaverRule::applyAction(ghidra::Funcdata &data) {
  for (const ActionStep &step : compiled.action.steps) {
    if (!applyActionStep(data, step, patternPnodes, patternVarnodes)) {
      return 0;
    }
  }
  return 1;
}

ghidra::int4 PcodeWeaverRule::apply(ghidra::Funcdata &data) {
  if (applyPattern(data) == 0) {
    return 0;
  }
  return applyAction(data);
}
