#pragma once

#include "parse/ast.hh"
#include "parse/ast_print.hh"

#include <cstdint>

namespace specvalues {
struct DefinedByIdNode {
  ast::Id nodeId;

  DefinedByIdNode() : nodeId() {}
  DefinedByIdNode(const ast::Id &id) : nodeId(id) {}

  auto operator<=>(const DefinedByIdNode &other) const {
    return this->nodeId <=> other.nodeId;
  };

  bool operator==(const DefinedByIdNode &other) const = default;

  size_t hash() const {
    size_t res = 0;
    hash_combine(res, nodeId);
    return res;
  }

  virtual const std::string &specName() const { return sSpecName; }
  static const inline std::string sSpecName = "defined_by";
};

struct SizeOfVarnode : DefinedByIdNode {
  SizeOfVarnode(const ast::Id &id) : DefinedByIdNode(id) {}

  const std::string &specName() const override { return sSpecName; }
  static inline const std::string sSpecName = "size_of_vn";
};

struct ConcreateSize {
  int64_t value;

  auto operator<=>(const ConcreateSize &other) const {
    return this->value <=> other.value;
  };
  bool operator==(const ConcreateSize &other) const = default;

  size_t hash() const {
    size_t res = 0;
    hash_combine(res, value);
    return res;
  }
};

using SpecValueSize = std::variant<ConcreateSize, SizeOfVarnode>;

struct OffsetOfVarnode : DefinedByIdNode {
  OffsetOfVarnode(const ast::Id &id) : DefinedByIdNode(id) {}

  const std::string &specName() const override { return sSpecName; }
  static inline const std::string sSpecName = "offset_of_vn";
};

struct ConcreateOffset {
  std::uint64_t value;

  auto operator<=>(const ConcreateOffset &other) const {
    return this->value <=> other.value;
  };
  bool operator==(const ConcreateOffset &other) const = default;

  size_t hash() const {
    size_t res = 0;
    hash_combine(res, value);
    return res;
  }
};

using SpecValueOffset = std::variant<ConcreateOffset, OffsetOfVarnode>;

struct BBOfVarnode : DefinedByIdNode {
  BBOfVarnode() : DefinedByIdNode() {}
  BBOfVarnode(const ast::Id &id) : DefinedByIdNode(id) {}

  const std::string &specName() const override { return sSpecName; }
  static inline const std::string sSpecName = "bb_of_vn";
};
struct BBOfPnode : DefinedByIdNode {
  BBOfPnode() : DefinedByIdNode() {}
  BBOfPnode(const ast::Id &id) : DefinedByIdNode(id) {}

  const std::string &specName() const override { return sSpecName; }
  static inline const std::string sSpecName = "bb_of_op";
};

using SpecValueBB = std::variant<BBOfPnode, BBOfVarnode>;

std::ostream &operator<<(std::ostream &os, const DefinedByIdNode &val);
std::ostream &operator<<(std::ostream &os, const ConcreateSize &val);
std::ostream &operator<<(std::ostream &os, const ConcreateOffset &val);
std::ostream &operator<<(std::ostream &os, const SpecValueSize &val);
std::ostream &operator<<(std::ostream &os, const SpecValueOffset &val);
std::ostream &operator<<(std::ostream &os, const SpecValueBB &val);
} // namespace specvalues

using specvalues::operator<<;

namespace std {
template <> struct hash<specvalues::ConcreateSize> {
  size_t operator()(const specvalues::ConcreateSize &x) const {
    return x.hash();
  }
};

template <> struct hash<specvalues::DefinedByIdNode> {
  size_t operator()(const specvalues::DefinedByIdNode &x) const {
    return x.hash();
  }
};

template <> struct hash<specvalues::SizeOfVarnode> {
  size_t operator()(const specvalues::SizeOfVarnode &x) const {
    return x.hash();
  }
};

template <> struct hash<specvalues::OffsetOfVarnode> {
  size_t operator()(const specvalues::OffsetOfVarnode &x) const {
    return x.hash();
  }
};

template <> struct hash<specvalues::ConcreateOffset> {
  size_t operator()(const specvalues::ConcreateOffset &x) const {
    return x.hash();
  }
};

template <> struct hash<specvalues::BBOfVarnode> {
  size_t operator()(const specvalues::BBOfVarnode &x) const { return x.hash(); }
};

template <> struct hash<specvalues::BBOfPnode> {
  size_t operator()(const specvalues::BBOfPnode &x) const { return x.hash(); }
};
} // namespace std
