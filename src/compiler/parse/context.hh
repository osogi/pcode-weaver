// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#pragma once

#include "parse/ast.hh"
#include "parse/op_type_predefined.hh"

// generated headers
#include "parse/parser.hpp"

struct Context {
  Context()
      : varnodeVarFactory("_v_"), pnodeVarFactory("_o_"),
        basicBlockVarFactory("_b_"), sizeVarFactory("_s_"),
        opTypePredefFactory(defaultOpType, sizeVarFactory) {
    location.initialize();
  };

  ast::IdFactory varnodeVarFactory;
  ast::IdFactory pnodeVarFactory;
  ast::IdFactory basicBlockVarFactory;
  ast::IdFactory sizeVarFactory;
  OpTypeFactory opTypePredefFactory;
  yy::location location;
};