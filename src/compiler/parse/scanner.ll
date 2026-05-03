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

COPY { return yy::Parser::make_COPY(driver.location());}
LOAD { return yy::Parser::make_LOAD(driver.location());}
STORE { return yy::Parser::make_STORE(driver.location());}
BRANCH { return yy::Parser::make_BRANCH(driver.location());}
CBRANCH { return yy::Parser::make_CBRANCH(driver.location());}
BRANCHIND { return yy::Parser::make_BRANCHIND(driver.location());}
CALL { return yy::Parser::make_CALL(driver.location());}
CALLIND { return yy::Parser::make_CALLIND(driver.location());}
USERDEFINED { return yy::Parser::make_USERDEFINED(driver.location());}
RETURN { return yy::Parser::make_RETURN(driver.location());}
PIECE { return yy::Parser::make_PIECE(driver.location());}
SUBPIECE { return yy::Parser::make_SUBPIECE(driver.location());}
POPCOUNT { return yy::Parser::make_POPCOUNT(driver.location());}
LZCOUNT { return yy::Parser::make_LZCOUNT(driver.location());}
INT\_EQUAL { return yy::Parser::make_INT_EQUAL(driver.location());}
INT\_NOTEQUAL { return yy::Parser::make_INT_NOTEQUAL(driver.location());}
INT\_LESS { return yy::Parser::make_INT_LESS(driver.location());}
INT\_SLESS { return yy::Parser::make_INT_SLESS(driver.location());}
INT\_LESSEQUAL { return yy::Parser::make_INT_LESSEQUAL(driver.location());}
INT\_SLESSEQUAL { return yy::Parser::make_INT_SLESSEQUAL(driver.location());}
INT\_ZEXT { return yy::Parser::make_INT_ZEXT(driver.location());}
INT\_SEXT { return yy::Parser::make_INT_SEXT(driver.location());}
INT\_ADD { return yy::Parser::make_INT_ADD(driver.location());}
INT\_SUB { return yy::Parser::make_INT_SUB(driver.location());}
INT\_CARRY { return yy::Parser::make_INT_CARRY(driver.location());}
INT\_SCARRY { return yy::Parser::make_INT_SCARRY(driver.location());}
INT\_SBORROW { return yy::Parser::make_INT_SBORROW(driver.location());}
INT\_2COMP { return yy::Parser::make_INT_2COMP(driver.location());}
INT\_NEGATE { return yy::Parser::make_INT_NEGATE(driver.location());}
INT\_XOR { return yy::Parser::make_INT_XOR(driver.location());}
INT\_AND { return yy::Parser::make_INT_AND(driver.location());}
INT\_OR { return yy::Parser::make_INT_OR(driver.location());}
INT\_LEFT { return yy::Parser::make_INT_LEFT(driver.location());}
INT\_RIGHT { return yy::Parser::make_INT_RIGHT(driver.location());}
INT\_SRIGHT { return yy::Parser::make_INT_SRIGHT(driver.location());}
INT\_MULT { return yy::Parser::make_INT_MULT(driver.location());}
INT\_DIV { return yy::Parser::make_INT_DIV(driver.location());}
INT\_REM { return yy::Parser::make_INT_REM(driver.location());}
INT\_SDIV { return yy::Parser::make_INT_SDIV(driver.location());}
INT\_SREM { return yy::Parser::make_INT_SREM(driver.location());}
BOOL\_NEGATE { return yy::Parser::make_BOOL_NEGATE(driver.location());}
BOOL\_XOR { return yy::Parser::make_BOOL_XOR(driver.location());}
BOOL\_AND { return yy::Parser::make_BOOL_AND(driver.location());}
BOOL\_OR { return yy::Parser::make_BOOL_OR(driver.location());}
FLOAT\_EQUAL { return yy::Parser::make_FLOAT_EQUAL(driver.location());}
FLOAT\_NOTEQUAL { return yy::Parser::make_FLOAT_NOTEQUAL(driver.location());}
FLOAT\_LESS { return yy::Parser::make_FLOAT_LESS(driver.location());}
FLOAT\_LESSEQUAL { return yy::Parser::make_FLOAT_LESSEQUAL(driver.location());}
FLOAT\_NAN { return yy::Parser::make_FLOAT_NAN(driver.location());}
FLOAT\_ADD { return yy::Parser::make_FLOAT_ADD(driver.location());}
FLOAT\_SUB { return yy::Parser::make_FLOAT_SUB(driver.location());}
FLOAT\_MULT { return yy::Parser::make_FLOAT_MULT(driver.location());}
FLOAT\_DIV { return yy::Parser::make_FLOAT_DIV(driver.location());}
FLOAT\_NEG { return yy::Parser::make_FLOAT_NEG(driver.location());}
FLOAT\_ABS { return yy::Parser::make_FLOAT_ABS(driver.location());}
FLOAT\_SQRT { return yy::Parser::make_FLOAT_SQRT(driver.location());}
FLOAT\_CEIL { return yy::Parser::make_FLOAT_CEIL(driver.location());}
FLOAT\_FLOOR { return yy::Parser::make_FLOAT_FLOOR(driver.location());}
FLOAT\_ROUND { return yy::Parser::make_FLOAT_ROUND(driver.location());}
INT2FLOAT { return yy::Parser::make_INT2FLOAT(driver.location());}
FLOAT2FLOAT { return yy::Parser::make_FLOAT2FLOAT(driver.location());}
TRUNC { return yy::Parser::make_TRUNC(driver.location());}
CPOOLREF { return yy::Parser::make_CPOOLREF(driver.location());}
NEW { return yy::Parser::make_NEW(driver.location());}


[oO][a-zA-Z0-9_]+  { return yy::Parser::make_PNODE_IDENTIFIER(yytext, driver.location()); }
[vV][a-zA-Z0-9_]+  { return yy::Parser::make_VARNODE_IDENTIFIER(yytext, driver.location()); }
[bB][a-zA-Z0-9_]+  { return yy::Parser::make_BASIC_BLOCK_IDENTIFIER(yytext, driver.location()); }

[a-zA-Z][a-zA-Z0-9_]* { return yy::Parser::make_IDENTIFIER(yytext, driver.location()); }

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
