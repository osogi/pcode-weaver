#pragma once

#include "validate/specvalues.hh"

namespace speccond {

struct SizeEqual {
  specvalues::SpecValueSize a;
  specvalues::SpecValueSize b;
};
struct BBEqual {
  specvalues::SpecValueBB a;
  specvalues::SpecValueBB b;
};
struct BBDominate {
  specvalues::SpecValueBB a;
  specvalues::SpecValueBB b;
};

using SpecCondition = std::variant<SizeEqual, BBEqual, BBDominate>;

std::ostream &operator<<(std::ostream &os, const SizeEqual &val);
std::ostream &operator<<(std::ostream &os, const BBEqual &val);
std::ostream &operator<<(std::ostream &os, const BBDominate &val);
std::ostream &operator<<(std::ostream &os, const SpecCondition &val);

} // namespace speccond

using speccond::operator<<;