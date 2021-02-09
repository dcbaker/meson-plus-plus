# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

prog_flex = find_program('flex')
prog_bison = find_program('bison')
cc = meson.get_compiler('c')

# Do we really want to use C for this, or C++
gen_flex = generator(
  prog_flex,
  arguments : ['--outfile=@OUTPUT@', '@INPUT@'],
  output : '@PLAINNAME@.c',
)

libfrontend = executable(
  'frontend',
  [
    gen_flex.process('interpreter.l'),
  ],
  dependencies : [
    cc.find_library('fl'),
  ],
)