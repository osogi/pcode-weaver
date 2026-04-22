/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>
 * Contains code adapted from the Ghidra project (Apache-2.0).
 * Contains code adapted from the bison-flex-cpp-example by Krzysztof Narkiewicz <krzysztof.narkiewicz@ezaquarii.com> (MIT).
 */

%option c++
%option nodefault
%option noyywrap
%option yyclass="Scanner"

%{
  #include <sstream>
  #include <iomanip>
  #include <iostream>
  #include "parse/driver.hh"

  #include "parse/scanner.hh"
  #include "parse/parser.hh"

  // Original yyterminate() macro returns int. Since we're using Bison 3 variants
  // as tokens, we must redefine it to change type from `int` to `Parser::semantic_type`
  #define yyterminate() Parser::make_END(driver.location());

  #define YY_USER_ACTION driver.locationAdvanceToken(yyleng);
  
  #define SCAN_NUMBER(numtext, cont)                                           \
  {                                                                            \
    std::istringstream s(numtext);                                             \
    int64_t val;                                                               \
    s >> std::setbase(0) >> val;                                               \
    if (!s) {                                                                  \
      std::ostringstream s;                                                    \
      s << "Scanner: couldn't parse number [" << numtext << "]";               \
      driver.error(driver.location(), s.str());                                \
    } else {                                                                   \
      return cont(val, driver.location());                                     \
    }                                                                          \
  }
%}

%%
\-\>   { return yy::Parser::make_RIGHT_ARROW(driver.location()); }
\-\>\> { return yy::Parser::make_DOUBLE_RIGHT_ARROW(driver.location()); }
\<\=   { return yy::Parser::make_DOMINATE(driver.location()); }
[\_]+  { return yy::Parser::make_UNDERSCORE(driver.location()); }
\-\-   { return yy::Parser::make_ACTION_TICK(driver.location()); }

AFTER   { return yy::Parser::make_AFTER_KEYWORD(driver.location()); }
BEFORE  { return yy::Parser::make_BEFORE_KEYWORD(driver.location()); }
EMPTY   { return yy::Parser::make_EMPTY_KEYWORD(driver.location()); }
NoOut   { return yy::Parser::make_NO_OUT_KEYWORD(driver.location()); }
NoOutOr { return yy::Parser::make_NO_OUT_OR_KEYWORD(driver.location()); }
DELETE   { return yy::Parser::make_DELETE_KEYWORD(driver.location()); }

(TRUE)|(True)|(true)    { return yy::Parser::make_TRUE_KEYWORD(driver.location()); }
(FALSE)|(False)|(false) { return yy::Parser::make_FALSE_KEYWORD(driver.location()); }

\, { return yy::Parser::make_COMMA(driver.location()); }
\; { return yy::Parser::make_SEMICOLON(driver.location()); }
\* { return yy::Parser::make_ASTERISK(driver.location()); }
\( { return yy::Parser::make_LPAREN(driver.location()); }
\) { return yy::Parser::make_RPAREN(driver.location()); }
\[ { return yy::Parser::make_LBRACK(driver.location()); }
\] { return yy::Parser::make_RBRACK(driver.location()); }

INT\_ADD { return yy::Parser::make_INT_ADD(driver.location());}
INT\_SUB { return yy::Parser::make_INT_SUB(driver.location());}


[oO][a-zA-Z0-9_]+  { return yy::Parser::make_PNODE_IDENTIFIER(yytext, driver.location()); }
[vV][a-zA-Z0-9_]+  { return yy::Parser::make_VARNODE_IDENTIFIER(yytext, driver.location()); }
[bB][a-zA-Z0-9_]+  { return yy::Parser::make_BASIC_BLOCK_IDENTIFIER(yytext, driver.location()); }

[a-zA-Z][[a-zA-Z0-9_]+] { return yy::Parser::make_IDENTIFIER(yytext, driver.location()); }

[#]\-?[0-9]+          { SCAN_NUMBER(yytext + 1,  yy::Parser::make_CONST); }
[#]\-?0x[0-9a-fA-F]+  { SCAN_NUMBER(yytext + 1,  yy::Parser::make_CONST); }

[0-9]+     { SCAN_NUMBER(yytext,  yy::Parser::make_NUMBER); }
0x[0-9a-fA-F]+  { SCAN_NUMBER(yytext,  yy::Parser::make_NUMBER); }

[\r\t\f\v ] { } //ignore it
\n { driver.locationAdvanceNewLine(); }

. {
    std::ostringstream s;
    s << "Scanner: unknown character [" << yytext << "]";
    driver.error(driver.location(), s.str());
  }

<<EOF>>     { return yyterminate(); }

%%