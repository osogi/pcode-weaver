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

} // namespace speccond