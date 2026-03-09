project(
  'pcode-weaver-publisher',
  'cpp',
  meson_version: '>=1.10.0',  # codegen module
  default_options: [
    'cpp_std=c++23', # std::expected
  ],
)

cc = meson.get_compiler('cpp')

    
inc = [
    include_directories('.'),
    '/home/user1/my_bins/reoxide/venv/lib/python3.12/site-packages/reoxide/data/include',
]

subdir('parse')
