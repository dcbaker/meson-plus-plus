# SPDX-License-Identifier: Apache-2.0
# Copyright © 2025 Intel Corporation

libmir = static_library(
  'mir',
  [
    'ir/basicblock.cpp',
    'ir/graph.cpp',
    'ir/instruction.cpp',
    'ir/message.cpp',
    'ir/operation.cpp',
    'ir/projectstate.cpp',
    'ir/state.cpp',
    'ir/target.cpp',
    'ir/number.cpp',
    'ir/string.cpp',
    'ir/identifier.cpp',
  ],
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
