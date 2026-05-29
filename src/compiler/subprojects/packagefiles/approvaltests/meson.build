project(
  'approvaltests',
  'cpp',
  version: '10.13.0',
)

approvaltests_sources = run_command(
  'find',
  'ApprovalTests',
  '-name',
  '*.cpp',
  check: true,
).stdout().strip().split('\n')

approvaltests_inc = [
  include_directories('.'),
  include_directories('ApprovalTests'),
]

approvaltests_lib = static_library(
  'approvaltests',
  approvaltests_sources,
  include_directories: approvaltests_inc,
)

approvaltests_dep = declare_dependency(
  include_directories: approvaltests_inc,
  link_with: approvaltests_lib,
)

meson.override_dependency('approvaltests', approvaltests_dep)
