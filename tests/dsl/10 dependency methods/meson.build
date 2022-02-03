project('dependency methods')

d = declare_dependency(version : '1.2.3')
assert(d.found())
assert(d.version() == '1.2.3')
assert(d.name() == 'internal')
