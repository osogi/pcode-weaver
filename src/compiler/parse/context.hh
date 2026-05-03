#pragma once

#include "parse/ast.hh"
#include "parse/op_type_predefined.hh"

// generated headers
#include "parse/parser.hh"


struct Context{
    Context() : 
    varnodeVarFactory("_v_"), 
    pnodeVarFactory("_o_"),
    basicBlockVarFactory("_b_"),
    sizeVarFactory("_s_"),
    opTypePredefFactory(defaultOpType, sizeVarFactory)
    {
        location.initialize();
    };

    ast::IdFactory varnodeVarFactory;
    ast::IdFactory pnodeVarFactory;
    ast::IdFactory basicBlockVarFactory;
    ast::IdFactory sizeVarFactory;
    OpTypeFactory opTypePredefFactory;
    yy::location location;
};