#include "parse/driver.hh"
#include "parse/ast_print.hh"
#include "validate/solvers.hh"
#include <iostream>

std::ostream &operator<<(std::ostream &os, const Errorable<bool> &err) {
  if(err.has_value()){
    os << err.value();
  }else{
    os << "Error: " << err.error().message();
  }

  return os;
}

int main(){
    ast::Id under("_", 1, false);
    ast::Id a("a", 2, true);
    ast::Id b("b", 3, true);
    

    solvers::EqualitySolver eq;
    std::cout << eq.addEquation(a, b) << "\n";
    std::cout << eq.find(a) << "\n";
    std::cout << eq.find(b) << "\n";

    std::cout << "\n" << eq.addEquation(a, 6) << "\n";
    std::cout << eq.find(a) << "\n";
    std::cout << eq.find(b) << "\n";

    std::cout << "\n"  << eq.addEquation(under, 6) << "\n";
    std::cout << eq.find(a) << "\n";
    std::cout << eq.find(b) << "\n";
    std::cout << eq.find(under) << "\n";

    std::cout << "\n"  << eq.addEquation(under, a) << "\n";
    std::cout << eq.find(a) << "\n";
    std::cout << eq.find(b) << "\n";
    std::cout << eq.find(under) << "\n";

    std::cout <<"\n -------------------  \n";


    solvers::PartialOrderSolver pos;
    ast::Id c("c", 3, true);
    ast::Id d("d", 4, true);

    std::cout << pos.addLessOrEqual(a, b) << "\n";
    std::cout << pos.addLessOrEqual(b, c) << "\n";
    std::cout << pos.isLessOrEqual(a, c) << "\n";
    std::cout << pos.isEqual(a, c) << "\n";

    std::cout << "\n" << pos.addLessOrEqual(b, under) << "\n";
    std::cout << pos.addLessOrEqual(under, d) << "\n";
    std::cout << pos.isLessOrEqual(c, d) << "\n";
    std::cout << pos.isLessOrEqual(b, d) << "\n";


    std::cout << "\n" << pos.addLessOrEqual(d, b) << "\n";
    std::cout << pos.isEqual(b, d) << "\n";
    std::cout << pos.isEqual(under, d) << "\n";
    std::cout << pos.isEqual(a, d) << "\n";
    std::cout << pos.isLessOrEqual(a, c) << "\n";
    std::cout << pos.isLessOrEqual(a, under) << "\n";




}