// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "validate/specconditions.hh"

namespace speccond {

std::ostream &operator<<(std::ostream &os, const SizeEqual &val) {
  return os << val.a << "==" << val.b;
};
std::ostream &operator<<(std::ostream &os, const OffsetEqual &val) {
  return os << val.a << "==" << val.b;
};
std::ostream &operator<<(std::ostream &os, const BBEqual &val) {
  return os << val.a << "==" << val.b;
}
std::ostream &operator<<(std::ostream &os, const BBDominate &val) {
  return os << val.a << "<=" << val.b;
}
std::ostream &operator<<(std::ostream &os, const BBOutgoingEdge &val) {
  return os << val.a << "->(" << val.outIndex << ")" << val.b;
}
std::ostream &operator<<(std::ostream &os, const SpecCondition &val) {
  return printVariant(os, val);
};
} // namespace speccond
