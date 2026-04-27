#include "parse/ast_print.hh"
#include "parse/driver.hh"
#include "validate/pcodegraph.hh"
#include "validate/solvers.hh"
#include <iostream>

std::ostream &operator<<(std::ostream &os, const Errorable<bool> &err) {
  if (err.has_value()) {
    os << err.value();
  } else {
    os << "Error: " << err.error().message();
  }

  return os;
}

int main() {
  ast::Id under("_", 1, false);
  ast::Id a("a", 2, true);
  ast::Id b("b", 3, true);

  ast::Id va("va", 2, true);
  ast::Id vb("vb", 3, true);
  specvalues::SizeOfVarnode varA(va);
  specvalues::SizeOfVarnode varB(vb);

  solvers::SizeSolver eq;
  std::cout << eq.addEquation(a, b) << "\n";
  std::cout << eq.find(a) << "\n";
  std::cout << eq.find(b) << "\n";

  std::cout << "\n" << eq.addEquation(a, varB) << "\n";
  std::cout << eq.find(a) << "\n";
  std::cout << eq.find(b) << "\n";
  std::cout << eq.find(varA) << "\n";
  std::cout << eq.find(varB) << "\n";

  std::cout << "\n" << eq.addEquation(varA, varB) << "\n";
  std::cout << eq.find(a) << "\n";
  std::cout << eq.find(b) << "\n";
  std::cout << eq.find(varA) << "\n";
  std::cout << eq.find(varB) << "\n";

  std::cout << "\n"
            << eq.addEquation(under, specvalues::ConcreateSize(6)) << "\n";
  std::cout << eq.find(a) << "\n";
  std::cout << eq.find(b) << "\n";
  std::cout << eq.find(varA) << "\n";
  std::cout << eq.find(varB) << "\n";
  std::cout << eq.find(under) << "\n";

  std::cout << "\n" << eq.addEquation(varB, under) << "\n";
  std::cout << eq.find(a) << "\n";
  std::cout << eq.find(b) << "\n";
  std::cout << eq.find(varA) << "\n";
  std::cout << eq.find(under) << "\n";

  std::cout << "\n"
            << eq.addEquation(varA, specvalues::ConcreateSize(7)) << "\n";
  std::cout << eq.find(a) << "\n";
  std::cout << eq.find(b) << "\n";
  std::cout << eq.find(varA) << "\n";
  std::cout << eq.find(under) << "\n";

  std::cout << "\n -------------------  \n";

  solvers::BBSolver pos;

  ast::Id pc("oc", 3, true);
  ast::Id vd("vd", 4, true);

  specvalues::BBOfPnode c(pc);
  specvalues::BBOfVarnode d(vd);

  std::cout << pos.addLessOrEqual(a, b) << "\n"; // 1
  std::cout << pos.addLessOrEqual(b, c) << "\n"; // 1
  std::cout << pos.isLessOrEqual(a, c) << "\n";  // 1
  std::cout << pos.isEqual(a, c) << "\n";        // 0

  std::cout << pos.find(a) << "\n";
  std::cout << pos.find(b) << "\n";
  std::cout << pos.find(c) << "\n";
  std::cout << pos.find(d) << "\n";

  std::cout << "\n" << pos.addLessOrEqual(b, under) << "\n"; // 1
  std::cout << pos.addLessOrEqual(under, d) << "\n";         // 1
  std::cout << pos.isLessOrEqual(c, d) << "\n";              // 0
  std::cout << pos.isLessOrEqual(b, d) << "\n";              // 1

  std::cout << pos.find(a) << "\n";
  std::cout << pos.find(b) << "\n";
  std::cout << pos.find(c) << "\n";
  std::cout << pos.find(d) << "\n";
  std::cout << pos.find(under) << "\n";

  std::cout << "\n" << pos.addLessOrEqual(d, b) << "\n"; // 1
  std::cout << pos.isEqual(b, d) << "\n";                // 1
  std::cout << pos.isEqual(under, d) << "\n";            // 1
  std::cout << pos.isEqual(a, d) << "\n";                // 0
  std::cout << pos.isLessOrEqual(a, c) << "\n";          // 1
  std::cout << pos.isLessOrEqual(a, under) << "\n";      // 1

  std::cout << pos.find(a) << "\n";
  std::cout << pos.find(b) << "\n";
  std::cout << pos.find(c) << "\n";
  std::cout << pos.find(d) << "\n";
  std::cout << pos.find(under) << "\n";
}