#pragma once

#include "parse/ast.hh"

#include <iosfwd>

namespace ast::print {
void rule(std::ostream &os, const Rule &r);
}

namespace ast {
std::ostream &operator<<(std::ostream &os, const ast::Id &id);
std::ostream &operator<<(std::ostream &os, const ast::Rule &r);
} // namespace ast