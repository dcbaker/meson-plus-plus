project('foo')

find_program('made up program', required : false)
find_program('sh')
find_program('cat')
find_program('echo')
find_program('sh')
find_program('sh', 'bash', 'dash', 'zsh')
find_program('made up program', 'ls')

x = find_program('sh')
assert(x.found(), 'sh was not found?')

y = find_program('not found', required : false)
assert(not y.found(), '"not found" was found?')
