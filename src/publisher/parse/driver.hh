#pragma once

#include "parse/context.hh"
#include "parse/scanner.hh"
#include "parse/op_type_predefined.hh"


// generated headers
#include "parse/parser.hh"

namespace yy {
class Driver {
  public:
    Driver();

    /**
     * Run parser. Results are stored inside.
     * \returns 0 on success, 1 on failure
     */
    int parse();

    // Return parsed Rule
    const ast::Rule& getParsedRule();

    friend Scanner;
    friend Parser;

  private:
    // location update methods
    void locationAdvanceToken(size_t amount);
    void locationAdvanceNewLine();

    // get current location
    const yy::location& location() const;

    // print error msg
    void error(const std::string &message);
    void error(const yy::location &loc, const std::string &message);

    Errorable<ast::OpTypePredefined>  getOpTypeByToken(OpTypeToken token);

    // Set rule upon completion of parsing
    void setParsedRule(ast::Rule &&rule);

    ast::Rule parsedRule;

    Context cntx;
    OpTypePredefinedFactory opTypePredefFactory;

    Scanner scanner;
    Parser parser;
};
} // namespace yy