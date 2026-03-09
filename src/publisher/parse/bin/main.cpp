#include "parse/driver.hh"
#include "parse/ast_print.hh"
#include <iostream>


int main(){
    yy::Driver drv;

    int parseRes = drv.parse();
    if(!parseRes){
        const auto &rule = drv.getParsedRule();
        std::cout << "Parsed Result:\n" << rule << "\n";
    }else{
        std::cout << "Error detected!";
    }
}