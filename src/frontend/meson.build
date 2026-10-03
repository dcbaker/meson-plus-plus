# SPDX-License-Identifier: Apache-2.0
# Copyright © 2021-2025 Intel Corporation

prog_bison = find_program('bison', version : '>= 3.2')
prog_flex = find_program('flex')

parser = custom_target(
  'parser.[ch]pp',
  input : 'parser.yy',
  output : ['@PLAINNAME@.cpp', '@PLAINNAME@.hpp', 'locations.hpp'],
  command : [prog_bison, '-o', '@OUTPUT0@', '--defines=@OUTPUT1@', '@INPUT@'],
)

locations_hpp = parser[2]

scanner = custom_target(
  'lexer.cpp',
  input : 'lexer.l',
  output : '@BASENAME@.cpp',
  command : [prog_flex, '-o', '@OUTPUT@', '@INPUT@'],
)

if not meson.get_compiler('cpp').has_header('FlexLexer.h')
  error('FlexLexer.h header not found, maybe install flex-dev?')
endif

# Bison generates switches that don't handle all of their enum values
# We can't fix that, so we have to ignore it.
#
# Because of this we want to be sure that we don't propogate the parser header
# out of this bit of code
libfrontend = static_library(
  'frontend',
  [parser, scanner, 'node.cpp', 'subdir_visitor.cpp', 'driver.cpp'],
  cpp_args : cpp.get_supported_arguments(
    '-Wno-switch-enum',
    '-Wno-implicit-fallthrough',
  ),
  dependencies : [dep_fs, idep_util],
)

inc_frontend = include_directories('.')

idep_frontend = declare_dependency(
  link_with : libfrontend,
  include_directories : inc_frontend,
)

test(
  'parser',
  executable(
    'parser_test',
    ['parser_test.cpp', parser[1]],
    link_with : libfrontend,
    dependencies : dep_gtest,
  ),
  protocol : 'gtest',
)

executable(
  'standalone_parser',
  ['standalone.cpp', parser[1]],
  link_with : libfrontend,
)
