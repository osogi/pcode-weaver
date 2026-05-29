approvaltests_dep = dependency(
  'approvaltests',
  fallback: ['approvaltests', 'approvaltests_dep'],
)

golden_common_src = ['golden_common.cpp']

parser_golden_test = executable(
  'parser_golden_test',
  golden_common_src + [
    'parser_golden_test.cpp',
  ],
  include_directories: [
    inc,
  ],
  dependencies: [
    parser_lib_dep,
    approvaltests_dep,
  ],
)

validate_golden_test = executable(
  'validate_golden_test',
  golden_common_src + [
    'validate_golden_test.cpp',
  ],
  include_directories: [
    inc,
  ],
  dependencies: [
    parser_lib_dep,
    validator_lib_dep,
    approvaltests_dep,
  ],
)

rulecompile_golden_test = executable(
  'rulecompile_golden_test',
  golden_common_src + [
    'rulecompile_golden_test.cpp',
  ],
  include_directories: [
    inc,
  ],
  dependencies: [
    parser_lib_dep,
    validator_lib_dep,
    rulecompile_lib_dep,
    approvaltests_dep,
  ],
)

golden_tests = {
  'parser': parser_golden_test,
  'validate': validate_golden_test,
  'rulecompile': rulecompile_golden_test,
}

foreach stage, runner : golden_tests
  fixtures_find = run_command(
    'find',
    'fixtures' / stage,
    '-type',
    'f',
    '-name',
    '*.rule',
    check: true,
  )

  fixtures = fixtures_find.stdout().strip().split('\n')

  foreach fixture : fixtures
    fixture_name = stage + ' / ' + fixture.split('/')[-1]
    test(
      fixture_name,
      runner,
      args: [fixture],
      workdir: meson.current_source_dir(),
    )
  endforeach
endforeach
