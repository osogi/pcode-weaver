/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>
 * Contains code adapted from bison-flex-cpp-example by Krzysztof Narkiewicz
 * <krzysztof.narkiewicz@ezaquarii.com>
 */

#pragma once

// generated headers
#include "parse/parser.hh"

#undef yyFlexLexer
#include <FlexLexer.h>

#undef YY_DECL
#define YY_DECL yy::Parser::symbol_type yy::Scanner::get_next_token()

namespace yy {
class Driver; // forward declaration

class Scanner : public yyFlexLexer {
public:
  Scanner(Driver &driver) : driver(driver) {}
  virtual ~Scanner() {}
  virtual Parser::symbol_type get_next_token();

private:
  Driver &driver;
};
} // namespace yy