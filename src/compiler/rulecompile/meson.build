rulecompile_src = [
  'compile_action.cpp',
  'compile_pattern.cpp',
  'rulecompile_driver.cpp',
]

rulecompile_lib = static_library(
  'rulecompile',
  rulecompile_src,
  include_directories : inc,
  dependencies: validator_lib_dep,
)

rulecompile_lib_dep = declare_dependency(
  include_directories : inc,
  link_with : rulecompile_lib,
  dependencies: validator_lib_dep,
)
