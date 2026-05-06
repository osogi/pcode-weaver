#pragma once

#include "cereal/types/common.hpp"
#include "cereal/types/vector.hpp"

#include <cstdint>
#include <vector>

namespace pcodeweaver::compiled {

using StepId = std::uint32_t;

enum class StepKind : std::uint8_t {
  AnyVarnode,
  Constant,
  Empty,
  Pnode,
};

enum class SourceKind : std::uint8_t {
  RootPnode,
  PnodeInput,
  PnodeOutput,
  VarnodeDef,
  VarnodeDescend,
};

enum class CheckKind : std::uint8_t {
  SizeEqual,
  OffsetEqual,
  BasicBlockEqual,
  BasicBlockDominates,
};

enum class CheckValueKind : std::uint8_t {
  ConstantSize,
  ConstantOffset,
  VarnodeSize,
  VarnodeBasicBlock,
  PnodeBasicBlock,
  VarnodeOffset,
};

enum class ActionNodeKind : std::uint8_t {
  Varnode,
  Pnode,
  Empty,
  Constant,
};

enum class ActionStepKind : std::uint8_t {
  CreateVarnode,
  CreatePnode,
  SetPnodeOutput,
  SetPnodeInput,
  DeletePnode,
};

struct StepSource {
  SourceKind kind = SourceKind::RootPnode;
  StepId from = 0;
  std::uint32_t inputIndex = 0;

  template <class Archive> void serialize(Archive &ar) {
    ar(kind, from, inputIndex);
  }
};

struct CheckValue {
  CheckValueKind kind = CheckValueKind::ConstantSize;
  StepId step = 0;
  std::int64_t constant = 0;

  template <class Archive> void serialize(Archive &ar) {
    ar(kind, step, constant);
  }
};

struct Check {
  CheckKind kind = CheckKind::SizeEqual;
  CheckValue left;
  CheckValue right;

  template <class Archive> void serialize(Archive &ar) {
    ar(kind, left, right);
  }
};

struct EdgeCheck {
  StepSource source;
  StepId expected = 0;

  template <class Archive> void serialize(Archive &ar) {
    ar(source, expected);
  }
};

struct MatchStep {
  StepKind kind = StepKind::AnyVarnode;
  StepSource source;
  bool hasOpCode = false;
  std::int32_t opCode = 0;
  std::int64_t constant = 0;
  std::vector<EdgeCheck> edgeChecks;
  std::vector<Check> checks;

  template <class Archive> void serialize(Archive &ar) {
    ar(kind, source, hasOpCode, opCode, constant, edgeChecks, checks);
  }
};

struct PatternProgram {
  std::vector<MatchStep> steps;

  template <class Archive> void serialize(Archive &ar) {
    ar(steps);
  }
};

struct ActionNodeRef {
  ActionNodeKind kind = ActionNodeKind::Empty;
  StepId step = 0;
  std::int64_t constant = 0;

  template <class Archive> void serialize(Archive &ar) {
    ar(kind, step, constant);
  }
};

struct ActionStep {
  ActionStepKind kind = ActionStepKind::CreateVarnode;
  ActionNodeRef target;
  ActionNodeRef value;
  std::uint32_t inputIndex = 0;
  std::uint32_t inputCount = 0;
  bool insertBefore = false;
  bool hasOpCode = false;
  std::int32_t opCode = 0;
  bool hasSize = false;
  CheckValue size;

  template <class Archive> void serialize(Archive &ar) {
    ar(kind,
       target,
       value,
       inputIndex,
       inputCount,
       insertBefore,
       hasOpCode,
       opCode,
       hasSize,
       size);
  }
};

struct ActionProgram {
  std::vector<ActionStep> steps;

  template <class Archive> void serialize(Archive &ar) { ar(steps); }
};

struct Rule {
  std::uint32_t formatVersion = 1;
  PatternProgram pattern;
  ActionProgram action;

  template <class Archive> void serialize(Archive &ar) {
    ar(formatVersion, pattern, action);
  }
};

} // namespace pcodeweaver::compiled
