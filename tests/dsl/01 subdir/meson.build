project('subdir')

a = false
b = false

subdir('sub')

if true
  subdir('conditional sub')
endif

assert(a == true, 'Did not go into subdir')
assert(b == true, 'Did not go into conditional subdir')
