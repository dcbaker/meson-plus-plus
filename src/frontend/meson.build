# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

prog_flex = find_program('flex')
prog_bison = find_program('bison')

interpreter = custom_target(
  'interpreter.[ch]pp',
  command : [prog_bison, '--defines=@OUTPUT1@', '--output=@OUTPUT0@', '@INPUT@'],
  output : ['interpreter.tab.cpp', 'interpreter.tab.hpp'],
  input : 'interpreter.ypp',
)

# Do we really want to use C for this, or C++
lexer = custom_target(
  'interpreter-lex.yy.cpp',
  input : 'interpreter.l',
  output : '@BASENAME@.yy.cpp',
  command : [prog_flex, '--c++', '--outfile=@OUTPUT@', '@INPUT@'],
)

libfrontend = static_library(
  'frontend',
  [
    interpreter,
    lexer,
  ],
)

executable(
  'standalone_parser',
  ['standalone_parser.cpp', interpreter[1]],
  link_with : libfrontend,
  dependencies : [cpp.find_library('fl')]
)
