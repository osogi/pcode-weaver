project(
  'approvaltests',
  'cpp',
  version: '10.13.0',
)

approvaltests_sources_text = run_command(
  'find',
  'ApprovalTests',
  '-name',
  '*.cpp',
  check: true,
).stdout().strip()

approvaltests_sources = []
if approvaltests_sources_text != ''
  approvaltests_sources = approvaltests_sources_text.split('\n')
endif

approvaltests_inc = [
  include_directories('.'),
  include_directories('ApprovalTests'),
]

approvaltests_dep = declare_dependency(
  include_directories: approvaltests_inc,
)

if approvaltests_sources.length() != 0
  approvaltests_lib = static_library(
    'approvaltests',
    approvaltests_sources,
    include_directories: approvaltests_inc,
  )

  approvaltests_dep = declare_dependency(
    include_directories: approvaltests_inc,
    link_with: approvaltests_lib,
  )
endif

meson.override_dependency('approvaltests', approvaltests_dep)
