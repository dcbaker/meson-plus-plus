# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

prog_flex = find_program('flex')
prog_bison = find_program('bison')

# Do we really want to use C for this, or C++
gen_flex = generator(
  prog_flex,
  arguments : ['--c++', '--outfile=@OUTPUT@', '@INPUT@'],
  output : '@BASENAME@.yy.cpp',
)

libfrontend = executable(
  'frontend',
  [
    gen_flex.process('interpreter.l'),
  ],
  dependencies : [
    cpp.find_library('fl'),
  ],
)