#pragma once

#include "parse/context.hh"
#include "parse/op_type_predefined.hh"
#include "parse/scanner.hh"

// generated headers
#include "parse/parser.hh"

#include <filesystem>

namespace yy {
class Driver {
public:
  Driver();

  /**
   * Run parser. Results are stored inside.
   * \returns 0 on success, 1 on failure
   */
  int parse(const std::filesystem::path &targetFile);

  // Return parsed Rule
  const ast::Rule &getParsedRule();

  Context &getContext();

  friend Scanner;
  friend Parser;

private:
  void switch_streams(
      std::istream &newIn = std::cin, std::ostream &newErr = std::cerr
  );

  // location update methods
  void locationAdvanceToken(size_t amount);
  void locationAdvanceNewLine();

  // get current location
  const yy::location &location() const;

  // print error msg
  void error(const std::string &message);
  void error(const yy::location &loc, const std::string &message);
  Errorable<ast::OpType> getOpTypeByToken(OpTypeToken token);

  // Set rule upon completion of parsing
  void setParsedRule(ast::Rule &&rule);

  ast::Rule parsedRule;

  Context cntx;

  Scanner scanner;
  Parser parser;

  bool meetErrorDuringParse = false;
  std::string currentFile = "NO_SETTED";
  std::ostream &errStream;
};
} // namespace yy