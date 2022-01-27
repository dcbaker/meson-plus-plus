project('custom_target', 'cpp')

prog = find_program('python')

custom_target(
  'foo',
  output : ['out.txt'],
  command : [prog, '-c', 'import sys; f = open(sys.argv[1], "w"); f.write("text"); f.close()', '@OUTPUT@'],
)

gen = custom_target(
  'gen.[ch]',
  output : ['gen.cpp', 'gen.hpp'],
  command : [prog, files('gen.py'), '@OUTPUT0@', '@OUTPUT1@'],
)

executable('gen', gen, 'main.cpp')
