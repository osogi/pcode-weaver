// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include "parse/ast.hh"

namespace ast {

const Size *getSizeOutCond(const OutVarnodeCondition &cond) {
  return std::visit(
      util::overloaded{
          [&](const ast::OutVarnodeConditionDefault &cndDef) -> const Size * {
            return &cndDef.size;
          },
          [&](const ast::OutVarnodeConditionNoOutOr &cndOr) -> const Size * {
            return &cndOr.alt.size;
          },
          [&](const ast::OutVarnodeConditionNoOut &) -> const Size * {
            return nullptr;
          }
      },
      cond
  );
}
} // namespace ast