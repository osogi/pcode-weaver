/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>
 * Contains code adapted from bison-flex-cpp-example by Krzysztof Narkiewicz
 * <krzysztof.narkiewicz@ezaquarii.com>
 */

#include "parse/driver.hh"

#include <sstream>

namespace yy {
Driver::Driver() : cntx(), scanner(*this), parser(scanner, *this) {};

int Driver::parse() {
  cntx.location.initialize();
  return parser.parse();
}

const ast::Rule &Driver::getParsedRule() { return parsedRule; }

// Increase location by number of characters matched
void Driver::locationAdvanceToken(size_t amount) {
  cntx.location.step();
  cntx.location.columns(amount);
}

// Handle newlines
void Driver::locationAdvanceNewLine() {
  cntx.location.step();
  cntx.location.lines(1);
}

const yy::location &Driver::location() const { return cntx.location; }

void Driver::error(const std::string &message) {
  std::cout << "Error: " << message << std::endl;
}

void Driver::error(const yy::location &loc, const std::string &message) {
  std::ostringstream s;
  if (loc.begin.filename)
    s << *loc.begin.filename << ':';
  s << loc.begin.line << ':' << loc.begin.column << ": " << message;
  error(s.str());
}

Errorable<ast::OpType> Driver::getOpTypeByToken(OpTypeToken token) {
  return cntx.opTypePredefFactory.getOpType(token);
};

void Driver::setParsedRule(ast::Rule &&rule) { parsedRule = std::move(rule); };

} // namespace yy
