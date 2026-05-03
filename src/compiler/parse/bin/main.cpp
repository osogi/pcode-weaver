#include "parse/ast_print.hh"
#include "parse/driver.hh"
#include <iostream>

int main(int argc, char *argv[]) {
  yy::Driver drv;

  for (int i = 1; i < argc; i++) {

    int parseRes = drv.parse(argv[i]);
    if (!parseRes) {
      const auto &rule = drv.getParsedRule();
      std::cout << "Parsed Result:\n" << rule << "\n";
    } else {
      std::cout << "Error detected!";
    }
  }
}