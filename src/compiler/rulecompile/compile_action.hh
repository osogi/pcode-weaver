// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include "common/error.hh"
#include "compiled_rule.hh"
#include "parse/ast.hh"
#include "rulecompile/compile_pattern.hh"
#include "validate/action_pcodegraph.hh"
#include "validate/solvers.hh"

#include <vector>

namespace rulecompile {

Errorable<pcodeweaver::compiled::ActionProgram> compileAction(
    const std::vector<ast::RuleAction> &actions,
    const graph::ActionPcodeGraph &actionGraph,
    const solvers::SizeSolver &sizeSolver, const PatternCompileResult &pattern
);

} // namespace rulecompile
