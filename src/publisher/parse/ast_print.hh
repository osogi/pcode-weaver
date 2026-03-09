#pragma once

#include "parse/ast.hh"

#include <iosfwd>

namespace ast::print {
  void rule(std::ostream& os, const Rule& r);
}

std::ostream& operator<<(std::ostream& os, const ast::Rule& r);