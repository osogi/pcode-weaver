#include "parse/ast_print.hh"
#include "parse/driver.hh"
#include "validate/validate_driver.hh"

#include <iostream>

int main(int argc, char *argv[]) {
  yy::Driver drv;

  for (int i = 1; i < argc; i++) {

    int parseRes = drv.parse(argv[i]);
    if (!parseRes) {
      const auto &rule = drv.getParsedRule();
      std::cout << "Parsed Result:\n" << rule << "\n\n\n";

      ValidateDriver vdrv(drv.getContext());

      auto validateRes = vdrv.validate(rule);
      if (validateRes.has_value()) {
        std::cout << "Validate: Ok!\n";

        auto rtcVec = vdrv.getRTC();
        std::cout << "RTC:\n";
        for (const auto &c : rtcVec) {
          std::cout << "\t" << c << "\n";
        }
      } else {
        std::cout << "Validate Error: " << validateRes.error().message() << "\n";
      }
    } else {
      std::cout << "Error detected!\n";
    }
  }
}