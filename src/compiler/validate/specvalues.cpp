// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "validate/specvalues.hh"
#include "parse/ast_print.hh"

namespace specvalues {
std::ostream &operator<<(std::ostream &os, const DefinedByIdNode &val) {
  os << val.specName() << "_" << val.nodeId;
  return os;
}
std::ostream &operator<<(std::ostream &os, const SpecValueSize &val) {
  return printVariant(os, val);
};
std::ostream &operator<<(std::ostream &os, const SpecValueOffset &val) {
  return printVariant(os, val);
};
std::ostream &operator<<(std::ostream &os, const SpecValueBB &val) {
  return printVariant(os, val);
};
std::ostream &operator<<(std::ostream &os, const ConcreateSize &val) {
  os << val.value;
  return os;
}
std::ostream &operator<<(std::ostream &os, const ConcreateOffset &val) {
  os << val.value;
  return os;
}
} // namespace specvalues
