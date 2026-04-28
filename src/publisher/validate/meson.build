validator_src = [
  'solvers.cpp',
  'specvalues.cpp',
  'specconditions.cpp',
  'pcodegraph.cpp',
  'inferencer.cpp',
  'validate_driver.cpp'
]

validator_lib = static_library(
  'validator',
  validator_src,
  include_directories : inc,
  dependencies: parser_lib_dep,
)

validator_lib_dep = declare_dependency(
  include_directories : inc,
  link_with : validator_lib,
)
