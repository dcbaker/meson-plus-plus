project(
  '04 static and exe link',
  'cpp',
  version : '1.0',
)

lib = static_library('lib', 'lib.cpp', cpp_args : ['-Duseless'])

main = executable('main', 'main.cpp', link_with : lib)

test(
  'main',
  main,
  should_fail : true,
)
