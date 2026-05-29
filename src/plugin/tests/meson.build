project(
  'pcode-weaver-plugin-system-tests',
  'cpp',
  meson_version: '>=1.10.0',
  default_options: [
    'cpp_std=c++17',
  ],
)

approvaltests_dep = dependency(
  'approvaltests',
  fallback: ['approvaltests', 'approvaltests_dep'],
)

plugin_approval_test = executable(
  'plugin_approval_test',
  'plugin_approval_test.cpp',
  cpp_args: ['-DAPPROVAL_TESTS_DISABLE_FILE_MACRO_CHECK'],
  dependencies: [
    approvaltests_dep,
  ],
)

cases_find = run_command(
  'find',
  'fixtures',
  '-mindepth',
  '1',
  '-maxdepth',
  '1',
  '-type',
  'd',
  check: true,
)

cases = cases_find.stdout().strip().split('\n')

foreach case_dir : cases
  test(
    'plugin / ' + case_dir.split('/')[-1],
    plugin_approval_test,
    args: [case_dir],
    workdir: meson.current_source_dir(),
    timeout: 600,
  )
endforeach
