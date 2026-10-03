# SPDX-License-Identifier: Apache-2.0
# Copyright © 2025-2026 Intel Corporation

de_parser = custom_target(
  'deserialize.[ch]pp',
  input : 'deserialize.yy',
  output : ['@PLAINNAME@.cpp', '@PLAINNAME@.hpp', '@BASENAME@_loc.hpp'],
  command : [prog_bison, '-o', '@OUTPUT0@', '--defines=@OUTPUT1@', '@INPUT@'],
)

de_locations_hpp = parser[2]

de_lexer = custom_target(
  'deserialize.l.cpp',
  input : 'deserialize.l',
  output : '@PLAINNAME@.cpp',
  command : [prog_flex, '-o', '@OUTPUT@', '@INPUT@'],
)

libmir_deserialize = static_library(
  'mir_deserialize',
  [de_parser, de_lexer, 'driver.cpp'],
  cpp_args : cpp.get_supported_arguments(
    '-Wno-switch-enum',
    '-Wno-implicit-fallthrough',
  ),
)
