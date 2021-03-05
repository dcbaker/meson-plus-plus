# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

prog_bison = find_program('bison', version : '>= 3.0')
prog_flex = find_program('flex')

parser = custom_target(
  'parser.[ch]pp',
  input : 'parser.yy',
  output : ['@PLAINNAME@.cpp', '@PLAINNAME@.hpp', 'locations.hpp'],
  command : [prog_bison, '-o', '@OUTPUT0@', '--defines=@OUTPUT1@', '@INPUT@'],
)

scanner = custom_target(
  'lexer.cpp',
  input : 'lexer.l',
  output : '@BASENAME@.cpp',
  command : [prog_flex, '-o', '@OUTPUT@', '@INPUT@'],
)

libfrontend = static_library(
  'frontend',
  [parser, scanner, 'node.cpp'],
)

executable(
  'standalone_parser',
  ['standalone.cpp', parser[1]],
  link_with : libfrontend,
)
