# SPDX-License-Identifier: Apache-2.0
# Copyright © 2025-2026 Intel Corporation

subdir('deserializer')

libmir = static_library(
  'mir',
  [
    'ir/array.cpp',
    'ir/basicblock.cpp',
    'ir/boolean.cpp',
    'ir/dict.cpp',
    'ir/file.cpp',
    'ir/graph.cpp',
    'ir/identifier.cpp',
    'ir/instruction.cpp',
    'ir/message.cpp',
    'ir/number.cpp',
    'ir/operation.cpp',
    'ir/phi.cpp',
    'ir/projectstate.cpp',
    'ir/state.cpp',
    'ir/string.cpp',
    'ir/target.cpp',
    'ir/undefined.cpp',
  ],
)

idep_mir = declare_dependency(link_with : [libmir, libmir_deserialize])

test(
  'mir',
  executable(
    'mir_tests',
    ['ir/graph_test.cpp'],
    dependencies : [idep_mir, dep_gtest],
  ),
  protocol : 'gtest',
)
