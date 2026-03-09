codegen = import('unstable-codegen')


flex_gen = codegen.lex(implementations : ['flex'])
bison_gen = codegen.yacc(implementations : ['bison'])


lex_gen_res = flex_gen.generate('scanner.ll')
lex_scanner_src = lex_gen_res[0]

bison_gen_res = bison_gen.generate('parser.yy')
bison_parser_src = bison_gen_res[0]


parser_src = [
    lex_scanner_src,
    bison_parser_src,
    'driver.cpp',
    'ast.cpp',
    'ast_print.cpp',
    'op_type_predefined.cpp',
]

parser_lib = static_library(
  'parser',
  parser_src,
  include_directories : inc,
)

parser_lib_dep = declare_dependency(
  include_directories : inc,
  link_with : parser_lib,
)

subdir('bin')