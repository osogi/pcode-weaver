#include "validate/specconditions.hh"

namespace speccond {

std::ostream &operator<<(std::ostream &os, const SizeEqual &val) {
  return os << val.a << "==" << val.b;
};
std::ostream &operator<<(std::ostream &os, const BBEqual &val) {
  return os << val.a << "==" << val.b;
}
std::ostream &operator<<(std::ostream &os, const BBDominate &val) {
  return os << val.a << "<=" << val.b;
}
std::ostream &operator<<(std::ostream &os, const SpecCondition &val) {
  return printVariant(os, val);
};
} // namespace speccond