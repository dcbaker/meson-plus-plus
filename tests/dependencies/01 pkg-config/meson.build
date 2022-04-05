project(
  'pkg-config dependency',
  'cpp',
)

dep = dependency('zlib')
assert(dep.found())

main = executable(
  'main',
  'main.cpp',
  dependencies : dep,
)

test('main', main)
