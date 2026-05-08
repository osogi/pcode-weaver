project(
  'pcode-weaver-publisher',
  'cpp',
  meson_version: '>=1.10.0', # codegen module
  default_options: [
    'cpp_std=c++23', # std::expected
  ],
)

cc = meson.get_compiler('cpp')

if not cc.compiles(
  '''
  #include <expected>

  int main() {
    std::expected<int, int> value = 1;
    return *value;
  }
  ''',
  name: 'C++23 std::expected support',
)
  error('The compiler build needs C++23 std::expected support, such as GCC 13+ or Clang 17+ with a matching standard library.')
endif

reoxide = find_program('reoxide', required: true)
reoxide_dir = run_command(reoxide, 'print-data-dir', check: true).stdout().strip()

inc = [
  include_directories('.'),
  include_directories('../inc'),
  reoxide_dir / 'include',
]

subdir('parse')
subdir('validate')
subdir('rulecompile')

subdir('bin')
