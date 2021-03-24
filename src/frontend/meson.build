# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

prog_bison = find_program('bison', version : '>= 3.2')
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

# Bison generates swtiches tahat don't handle all of their enum values
# We can't fix that, so we have to ignore it.
#
# Because of this we want to be sure that we don't propogate the parser header
# out of thehis bit of code
_frontend_args = cpp.get_supported_arguments(
  '-Wno-switch-enum',
)

libfrontend = static_library(
  'frontend',
  [parser, scanner, 'node.cpp', 'node_visitors.cpp', 'driver.cpp'],
  cpp_args : _frontend_args,
)

test(
  'parser',
  executable(
    'parser_test',
    ['parser_test.cpp', parser[1]],
    cpp_args : _frontend_args,
    link_with : libfrontend,
    dependencies : dep_gtest,
  ),
  protocol : 'gtest',
)

executable(
  'standalone_parser',
  ['standalone.cpp', parser[1]],
  link_with : libfrontend,
  cpp_args : _frontend_args,
)
