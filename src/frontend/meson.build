# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

prog_flex = find_program('flex')
prog_bison = find_program('bison')

parser = custom_target(
  'parser.[ch]pp',
  command : [prog_bison, '--defines=@OUTPUT0@', '--output=@OUTPUT1@', '@INPUT@'],
  output : ['parser.hpp', 'parser.cpp'],
  input : 'interpreter.ypp',
)

# Do we really want to use C for this, or C++
gen_flex = generator(
  prog_flex,
  arguments : ['--c++', '--outfile=@OUTPUT@', '@INPUT@'],
  output : '@BASENAME@.yy.cpp',
)

libfrontend = executable(
  'frontend',
  [
    parser,
    gen_flex.process('interpreter.l'),
  ],
)
