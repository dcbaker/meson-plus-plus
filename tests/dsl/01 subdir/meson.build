project('subdir')

a = false
b = false

subdir('sub')

x = 'foo'
subdir(x)

if true
  subdir('conditional sub')
elif false
  subdir('elif sub')
elif 3 == 5
  subdir('elif sub2')
else
  subdir('else')
endif

foreach s : ['foreach']
  subdir(s)
endforeach

assert(a == true, 'Did not go into subdir')
assert(b == true, 'Did not go into conditional subdir')
