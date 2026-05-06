/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>
 * Contains code adapted from the Ghidra project (Apache-2.0).
 * Contains code adapted from the bison-flex-cpp-example by Krzysztof Narkiewicz <krzysztof.narkiewicz@ezaquarii.com> (MIT).
 */

%skeleton "lalr1.cc"
%require "3.8"

%defines "parse/parser.hh"

%define api.parser.class  { Parser }
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires
{
  #include <stdint.h>

  // forward declaration
  namespace yy{
    class Scanner;
    class Driver;
  }
}

%code top
{
  #include <iostream>
  #include <memory>

  #include "parse/ast.hh"
  #include "parse/op_type_predefined.hh"


  #include "parse/driver.hh"
  #include "parse/scanner.hh"
  #include "parse/parser.hh"

  static yy::Parser::symbol_type yylex(yy::Scanner &scanner) {
    return scanner.get_next_token();
  }
}

%lex-param { Scanner &scanner }
%parse-param { Scanner &scanner }
%parse-param { Driver &driver }


%locations
%define parse.trace
%define parse.error verbose

%define api.token.prefix {RULES_TOKEN_}

%token END 0 "end of file"
%token RIGHT_ARROW "->"
%token DOUBLE_RIGHT_ARROW "->>"
%token DOMINATE "<="
%token UNDERSCORE "_"
%token ACTION_TICK "--"

%token AFTER_KEYWORD "AFTER"
%token BEFORE_KEYWORD "BEFORE"
%token EMPTY_KEYWORD "EMPTY"
%token NO_OUT_KEYWORD "NoOut"
%token NO_OUT_OR_KEYWORD "NoOutOr"
%token TRUE_KEYWORD "True"
%token FALSE_KEYWORD "False"
%token DELETE_KEYWORD "DELETE"



%token COMMA ","
%token SEMICOLON ";"
%token ASTERISK "*"
%token LPAREN "("
%token RPAREN ")"
%token LBRACK "["
%token RBRACK "]"



%token COPY
%token LOAD
%token STORE
%token BRANCH
%token CBRANCH
%token BRANCHIND
%token CALL
%token CALLIND
%token USERDEFINED
%token RETURN
%token PIECE
%token SUBPIECE
%token POPCOUNT
%token LZCOUNT
%token INT_EQUAL
%token INT_NOTEQUAL
%token INT_LESS
%token INT_SLESS
%token INT_LESSEQUAL
%token INT_SLESSEQUAL
%token INT_ZEXT
%token INT_SEXT
%token INT_ADD
%token INT_SUB
%token INT_CARRY
%token INT_SCARRY
%token INT_SBORROW
%token INT_2COMP
%token INT_NEGATE
%token INT_XOR
%token INT_AND
%token INT_OR
%token INT_LEFT
%token INT_RIGHT
%token INT_SRIGHT
%token INT_MULT
%token INT_DIV
%token INT_REM
%token INT_SDIV
%token INT_SREM
%token BOOL_NEGATE
%token BOOL_XOR
%token BOOL_AND
%token BOOL_OR
%token FLOAT_EQUAL
%token FLOAT_NOTEQUAL
%token FLOAT_LESS
%token FLOAT_LESSEQUAL
%token FLOAT_NAN
%token FLOAT_ADD
%token FLOAT_SUB
%token FLOAT_MULT
%token FLOAT_DIV
%token FLOAT_NEG
%token FLOAT_ABS
%token FLOAT_SQRT
%token FLOAT_CEIL
%token FLOAT_FLOOR
%token FLOAT_ROUND
%token INT2FLOAT
%token FLOAT2FLOAT
%token TRUNC
%token CPOOLREF
%token NEW

%left RIGHT_ARROW
%left DOMINATE

%left DOUBLE_RIGHT_ARROW

%token <std::string> VARNODE_IDENTIFIER "varnode name"
%token <int64_t> CONST "constant"
%token <std::string> PNODE_IDENTIFIER "pnode name"
%token <std::string> BASIC_BLOCK_IDENTIFIER "basic block name"
%token <std::string> IDENTIFIER "variable name"
%token <uint64_t> NUMBER "number"

%type <int> op_type_token
%type <bool> boolean
%type <ast::Size> size
%type <ast::BasicBlockVar> bb_var
%type <ast::VarnodeType> varnode_type
%type <ast::VarnodeVar> varnode_var
%type <ast::VarnodeConst> varnode_const
%type <ast::PnodeVar> pnode_var
%type <ast::OpType> op_type
%type <ast::PnodeType> pnode_type

/*
%type <ast::InVarnodeConditionsSpecial> in_varnodes_conds_special
%type <ast::InVarnodeCondition> in_varnode_cond
%type <ast::InVarnodeConditionsArray> in_varnodes_conds_arr
%type <ast::InVarnodeConditions> in_varnodes_conds
%type <ast::OutVarnodeConditionDefault> out_varnode_cond_def
%type <ast::OutVarnodeConditionNoOutOr> out_varnode_cond_no_out_or
%type <ast::OutVarnodeConditionNoOut> out_varnode_cond_no_out
%type <ast::OutVarnodeCondition> out_varnode_cond
*/

%type <ast::PnodeTerm> pnode_term
%type <ast::VarnodeTerm> varnode_term
%type <ast::PnodePattern> pnode_pattern
%type <ast::VarnodePattern> varnode_pattern
%type <ast::BasicBlockPattern> basic_block_pattern
%type <ast::RulePattern> rule_pattern

%type <ast::VarnodeActionTerm> varnode_action_term
%type <ast::PnodeActionTerm> pnode_action_term
%type <ast::VarnodeAction> varnode_action
%type <ast::PnodeAction> pnode_action
%type <ast::EmptyAction> empty_action
%type <ast::RuleAction> rule_action

%type <std::vector<ast::RulePattern>> patterns
%type <std::vector<ast::RuleAction>> actions

%start fullrule

%%
// small help objects

boolean:
    TRUE_KEYWORD  { $$ = true; }
  | FALSE_KEYWORD { $$ = false; }

size: 
    IDENTIFIER  { $$ = driver.cntx.sizeVarFactory.createId($1, true); }
  | UNDERSCORE  { $$ = driver.cntx.sizeVarFactory.createId(false); }
  | NUMBER      { $$ = ghidra::int4($1);}

bb_var:
    BASIC_BLOCK_IDENTIFIER { $$ = ast::BasicBlockVar{driver.cntx.basicBlockVarFactory.createId($1, true)};}
  | UNDERSCORE             { $$ = ast::BasicBlockVar{driver.cntx.basicBlockVarFactory.createId(false)}; };

varnode_var:
  VARNODE_IDENTIFIER {$$ = ast::VarnodeVar{ driver.cntx.varnodeVarFactory.createId($1, true) };}

varnode_const:
  CONST { $$ = ast::VarnodeConst{ $1 };}

pnode_var:
  PNODE_IDENTIFIER {$$ = ast::PnodeVar{ driver.cntx.pnodeVarFactory.createId($1, true) };}


varnode_type:
    size COMMA bb_var { $$ = ast::VarnodeType{$1, $3};}
  | size COMMA bb_var COMMA NUMBER { $$ = ast::VarnodeType{$1, $3, $5};}

/*
in_varnode_cond:
  size { $$ = ast::InVarnodeCondition{$1}; }

in_varnodes_conds_arr:
    in_varnode_cond
      {
        $$ = ast::InVarnodeConditionsArray{};
        $$.array.push_back($1);
      }
  | in_varnodes_conds_arr COMMA in_varnode_cond
      {
        ast::InVarnodeConditionsArray &conds = $1;
        conds.array.push_back($3); 
        $$ = conds;
      }

in_varnodes_conds_special:
  ASTERISK { $$ = ast::InVarnodeConditionsSpecial{};}

out_varnode_cond_def:
  size { $$ = ast::OutVarnodeConditionDefault{$1}; }

out_varnode_cond_no_out_or:
  NO_OUT_OR_KEYWORD LPAREN out_varnode_cond_def RPAREN { $$ = ast::OutVarnodeConditionNoOutOr{$3}; }

out_varnode_cond_no_out:
  NO_OUT_KEYWORD { $$ = ast::OutVarnodeConditionNoOut{}; }

in_varnodes_conds:
    LBRACK in_varnodes_conds_arr RBRACK { $$ = $2; }
  | in_varnodes_conds_special           { $$ = $1; }

out_varnode_cond:
    out_varnode_cond_def        { $$ = $1; }
  | out_varnode_cond_no_out_or  { $$ = $1; }
  | out_varnode_cond_no_out     { $$ = $1; }
*/

// TODO: need to rework this
op_type_token:
    COPY { $$ = yy::Parser::token_kind_type::RULES_TOKEN_COPY; }
  | LOAD { $$ = yy::Parser::token_kind_type::RULES_TOKEN_LOAD; }
  | STORE { $$ = yy::Parser::token_kind_type::RULES_TOKEN_STORE; }
  | BRANCH { $$ = yy::Parser::token_kind_type::RULES_TOKEN_BRANCH; }
  | CBRANCH { $$ = yy::Parser::token_kind_type::RULES_TOKEN_CBRANCH; }
  | BRANCHIND { $$ = yy::Parser::token_kind_type::RULES_TOKEN_BRANCHIND; }
  | CALL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_CALL; }
  | CALLIND { $$ = yy::Parser::token_kind_type::RULES_TOKEN_CALLIND; }
  | USERDEFINED { $$ = yy::Parser::token_kind_type::RULES_TOKEN_USERDEFINED; }
  | RETURN { $$ = yy::Parser::token_kind_type::RULES_TOKEN_RETURN; }
  | PIECE { $$ = yy::Parser::token_kind_type::RULES_TOKEN_PIECE; }
  | SUBPIECE { $$ = yy::Parser::token_kind_type::RULES_TOKEN_SUBPIECE; }
  | POPCOUNT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_POPCOUNT; }
  | LZCOUNT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_LZCOUNT; }
  | INT_EQUAL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_EQUAL; }
  | INT_NOTEQUAL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_NOTEQUAL; }
  | INT_LESS { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_LESS; }
  | INT_SLESS { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SLESS; }
  | INT_LESSEQUAL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_LESSEQUAL; }
  | INT_SLESSEQUAL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SLESSEQUAL; }
  | INT_ZEXT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_ZEXT; }
  | INT_SEXT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SEXT; }
  | INT_ADD { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_ADD; }
  | INT_SUB { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SUB; }
  | INT_CARRY { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_CARRY; }
  | INT_SCARRY { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SCARRY; }
  | INT_SBORROW { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SBORROW; }
  | INT_2COMP { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_2COMP; }
  | INT_NEGATE { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_NEGATE; }
  | INT_XOR { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_XOR; }
  | INT_AND { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_AND; }
  | INT_OR { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_OR; }
  | INT_LEFT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_LEFT; }
  | INT_RIGHT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_RIGHT; }
  | INT_SRIGHT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SRIGHT; }
  | INT_MULT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_MULT; }
  | INT_DIV { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_DIV; }
  | INT_REM { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_REM; }
  | INT_SDIV { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SDIV; }
  | INT_SREM { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT_SREM; }
  | BOOL_NEGATE { $$ = yy::Parser::token_kind_type::RULES_TOKEN_BOOL_NEGATE; }
  | BOOL_XOR { $$ = yy::Parser::token_kind_type::RULES_TOKEN_BOOL_XOR; }
  | BOOL_AND { $$ = yy::Parser::token_kind_type::RULES_TOKEN_BOOL_AND; }
  | BOOL_OR { $$ = yy::Parser::token_kind_type::RULES_TOKEN_BOOL_OR; }
  | FLOAT_EQUAL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_EQUAL; }
  | FLOAT_NOTEQUAL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_NOTEQUAL; }
  | FLOAT_LESS { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_LESS; }
  | FLOAT_LESSEQUAL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_LESSEQUAL; }
  | FLOAT_NAN { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_NAN; }
  | FLOAT_ADD { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_ADD; }
  | FLOAT_SUB { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_SUB; }
  | FLOAT_MULT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_MULT; }
  | FLOAT_DIV { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_DIV; }
  | FLOAT_NEG { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_NEG; }
  | FLOAT_ABS { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_ABS; }
  | FLOAT_SQRT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_SQRT; }
  | FLOAT_CEIL { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_CEIL; }
  | FLOAT_FLOOR { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_FLOOR; }
  | FLOAT_ROUND { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT_ROUND; }
  | INT2FLOAT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_INT2FLOAT; }
  | FLOAT2FLOAT { $$ = yy::Parser::token_kind_type::RULES_TOKEN_FLOAT2FLOAT; }
  | TRUNC { $$ = yy::Parser::token_kind_type::RULES_TOKEN_TRUNC; }
  | CPOOLREF { $$ = yy::Parser::token_kind_type::RULES_TOKEN_CPOOLREF; }
  | NEW { $$ = yy::Parser::token_kind_type::RULES_TOKEN_NEW; }

op_type:
  op_type_token { 
                              auto res = driver.getOpTypeByToken($1);
                              if (res.has_value()){
                                $$ = res.value();
                              }else{
                                error(@1, res.error().message());
                              }
                            }

pnode_type:
  op_type COMMA bb_var { $$ = ast::PnodeType($1, $3); }

// main rules

fullrule: 
  patterns ACTION_TICK actions 
    {
      driver.setParsedRule(ast::Rule{std::move($1), std::move($3)});
    };

patterns:
  rule_pattern 
    {
      $$ = std::vector<ast::RulePattern>();
      $$.push_back(std::move($1));
    }
  | patterns SEMICOLON rule_pattern 
    {
      $$ = std::move($1);
      $$.push_back(std::move($3));
    }

actions:
  rule_action 
    {
      $$ = std::vector<ast::RuleAction>();
      $$.push_back(std::move($1));
    }
  | actions SEMICOLON rule_action 
    {
      $$ = std::move($1);
      $$.push_back(std::move($3));
    }


// Patterns

varnode_term:
    EMPTY_KEYWORD                           { $$ = ast::VarnodeEmpty{}; }
  | varnode_var                             { $$ = $1; }
  | varnode_var LPAREN  varnode_type RPAREN { $$ = ast::VarnodeVarWithType{$1, $3}; }
  | varnode_const                           { $$ = $1; }


pnode_term:
    EMPTY_KEYWORD                       { $$ = ast::PnodeEmpty{}; }
  | pnode_var                           { $$ = $1; }
  | pnode_var LPAREN  pnode_type RPAREN { $$ = ast::PnodeVarWithType{$1, $3}; };


varnode_pattern: 
    varnode_term 
      {
        $$ = $1;
      }
  | pnode_pattern[pp] RIGHT_ARROW varnode_term[vt] 
      {
        $$ = std::make_unique<ast::VarnodeDefedBy>(
          ast::VarnodeDefedBy{std::move($pp), $vt}
          );
      }

pnode_pattern: 
    pnode_term 
      {
        $$ = $1;
      }
  | varnode_pattern[vp] RIGHT_ARROW LPAREN NUMBER[num] RPAREN pnode_term[pt] 
      {
        $$ = std::make_unique<ast::PnodeThatTakeAsNthArg>(
          ast::PnodeThatTakeAsNthArg{std::move($vp), static_cast<uint32_t>($num), $pt}
          );
      }
/*
  | varnode_pattern[vp] RIGHT_ARROW pnode_term[pt] 
      {
        $$ = std::make_unique<ast::PnodeThatTakeAsSomeArg>(
          ast::PnodeThatTakeAsSomeArg{std::move($vp), $pt}
          );
      } 
*/

basic_block_pattern:
    bb_var 
      {
        $$ = $1;
      }
  | basic_block_pattern[bbp] DOMINATE bb_var[bbv]
      {
        $$ = std::make_unique<ast::BasicBlockDominatedBy>(
          ast::BasicBlockDominatedBy{std::move($bbp), $bbv}
          );
      }

rule_pattern:
    varnode_pattern      { $$ = std::move($1); }
  | pnode_pattern        { $$ = std::move($1); }
  | basic_block_pattern  { $$ = std::move($1); }

// Actions

varnode_action_term:
    EMPTY_KEYWORD                           { $$ = ast::VarnodeEmpty{}; }
  | varnode_var                             { $$ = $1; }
  | varnode_var[vv] LPAREN size[sz] RPAREN  { $$ = ast::VarnodeSpecSize{$vv, $sz}; }

pnode_action_term:
    pnode_var 
      { $$ = $1; }
  | pnode_var[new_pv] LPAREN op_type[op_tp] BEFORE_KEYWORD pnode_var[old_pv] RPAREN  
    { $$ =  ast::PnodeSpecTypeAndLoc{$new_pv, $op_tp, true, $old_pv}; }
  | pnode_var[new_pv] LPAREN op_type[op_tp] AFTER_KEYWORD pnode_var[old_pv] RPAREN  
    { $$ =  ast::PnodeSpecTypeAndLoc{$new_pv, $op_tp, false, $old_pv}; }

varnode_action:
    varnode_action_term 
      { $$ = $1; }
  | pnode_action[pa] DOUBLE_RIGHT_ARROW varnode_action_term[vt]
      { 
        $$ = std::make_unique<ast::VarnodeSetAsPnodeOut>(
          ast::VarnodeSetAsPnodeOut{std::move($pa), $vt}
        );
      }
  | varnode_const { $$ = $1; }

pnode_action:
    pnode_action_term { $$ = $1; }
  | varnode_action[va] DOUBLE_RIGHT_ARROW LPAREN NUMBER[num] RPAREN pnode_action_term[pt]
    {
      $$ = std::make_unique<ast::PnodeSetNthArg>(
          ast::PnodeSetNthArg{std::move($va), static_cast<uint32_t>($num), $pt}
        );
    }

empty_action:
  DELETE_KEYWORD pnode_var { $$ = ast::EmptyActionDeletePnode{$2}; }

rule_action:
    pnode_action   { $$ = std::move($1); }
  | varnode_action { $$ = std::move($1); }
  | empty_action   { $$ = std::move($1); }
%%



void yy::Parser::error(const location &loc , const std::string &message) {
  driver.error(loc, message);
}
