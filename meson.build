project(
  'plugin-template',
  'cpp',
  version: '0.0.1',
  meson_version: '>=1.10.0',  # codegen module
  default_options: [
    'cpp_std=c++17',
    'warning_level=1',
    'debug=true',
    'default_library=shared'
  ]
)

if host_machine.system() != 'linux'
  error('Currently only Linux is supported for ReOxide')
endif

reoxide = find_program('reoxide', required: true)
reoxide_dir = run_command(reoxide, 'print-data-dir', check: true).stdout().strip()
reoxide_plugin_dir = run_command(reoxide, 'print-plugin-dir', check: true).stdout().strip()
reoxide_bin_dir = reoxide_dir / 'bin'

cc = meson.get_compiler('cpp')
reoxide = declare_dependency(
  dependencies: [
    cc.find_library('reoxide', dirs: reoxide_bin_dir),
  ],
  include_directories: reoxide_dir / 'include'
)

# Ghidra has a lot of sign comparison issues. Until these get fixed,
# it is better to disable the warnings for everything that imports
# Ghidra code, because there will be too much noise otherwise.
if cc.get_argument_syntax() == 'gcc'
  add_project_arguments('-Wno-sign-compare', language: 'cpp')
endif

subdir('simple')
subdir('src/publisher')