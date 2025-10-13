# SPDX-License-Identifier: Apache-2.0
# Copyright © 2025 Intel Corporation

subdir('ir/deserializer')

libmir = static_library(
  'mir',
  [
    'ir/basicblock.cpp',
    'ir/boolean.cpp',
    'ir/graph.cpp',
    'ir/identifier.cpp',
    'ir/instruction.cpp',
    'ir/message.cpp',
    'ir/number.cpp',
    'ir/operation.cpp',
    'ir/projectstate.cpp',
    'ir/state.cpp',
    'ir/string.cpp',
    'ir/target.cpp',
    de_parser,
    de_lexer,
  ],
  cpp_args : ['-Wno-switch-enum'],
)

idep_mir = declare_dependency(link_with : libmir)

test(
  'mir',
  executable(
    'mir_tests',
    ['ir/graph_test.cpp'],
    dependencies : [idep_mir, dep_gtest],
  ),
  protocol : 'gtest',
)
