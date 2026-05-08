// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include "validate/specvalues.hh"

namespace speccond {

struct SizeEqual {
  specvalues::SpecValueSize a;
  specvalues::SpecValueSize b;
};
struct OffsetEqual {
  specvalues::SpecValueOffset a;
  specvalues::SpecValueOffset b;
};
struct BBEqual {
  specvalues::SpecValueBB a;
  specvalues::SpecValueBB b;
};
struct BBDominate {
  specvalues::SpecValueBB a;
  specvalues::SpecValueBB b;
};
struct BBOutgoingEdge {
  specvalues::SpecValueBB a;
  specvalues::SpecValueBB b;
  std::uint32_t outIndex;
};

using SpecCondition =
    std::variant<SizeEqual, OffsetEqual, BBEqual, BBDominate, BBOutgoingEdge>;

std::ostream &operator<<(std::ostream &os, const SizeEqual &val);
std::ostream &operator<<(std::ostream &os, const OffsetEqual &val);
std::ostream &operator<<(std::ostream &os, const BBEqual &val);
std::ostream &operator<<(std::ostream &os, const BBDominate &val);
std::ostream &operator<<(std::ostream &os, const BBOutgoingEdge &val);
std::ostream &operator<<(std::ostream &os, const SpecCondition &val);

} // namespace speccond

using speccond::operator<<;
