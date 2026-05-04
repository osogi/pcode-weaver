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
  BasicBlockEqual,
  BasicBlockDominates,
};

enum class CheckValueKind : std::uint8_t {
  ConstantSize,
  VarnodeSize,
  VarnodeBasicBlock,
  PnodeBasicBlock,
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

struct Rule {
  std::uint32_t formatVersion = 1;
  PatternProgram pattern;

  template <class Archive> void serialize(Archive &ar) {
    ar(formatVersion, pattern);
  }
};

} // namespace pcodeweaver::compiled
