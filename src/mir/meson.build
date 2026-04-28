# SPDX-License-Identifier: Apache-2.0
# Copyright © 2025-2026 Intel Corporation

subdir('deserializer')

libmir = static_library(
  'mir',
  [
    'ast_to_mir.cpp',
    'builder/builder.cpp',
    'ir/array.cpp',
    'ir/basicblock.cpp',
    'ir/boolean.cpp',
    'ir/cfg.cpp',
    'ir/dict.cpp',
    'ir/file.cpp',
    'ir/functioncall.cpp',
    'ir/graph.cpp',
    'ir/helpers.cpp',
    'ir/identifier.cpp',
    'ir/instruction.cpp',
    'ir/message.cpp',
    'ir/number.cpp',
    'ir/phi.cpp',
    'ir/projectstate.cpp',
    'ir/string.cpp',
    'ir/target.cpp',
    'ir/undefined.cpp',
    'passes/remove_ternary.cpp',
  ],
  include_directories : [inc_frontend],
  dependencies : [idep_util],
)

idep_mir = declare_dependency(link_with : [libmir, libmir_deserialize])

test(
  'mir',
  executable(
    'mir_tests',
    [
      'ast_to_mir_test.cpp',
      'builder/builder_test.cpp',
      'ir/graph_test.cpp',
      'passes/remove_ternary_test.cpp',
      'test_helpers.cpp',
    ],
    include_directories : '.',
    dependencies : [idep_mir, dep_gtest, idep_frontend],
  ),
  protocol : 'gtest',
)
