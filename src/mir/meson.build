# SPDX-License-Identifier: Apache-2.0
# Copyright © 2025 Intel Corporation

libmir = static_library(
  'mir',
  [
    'graph.cpp',
    'message.cpp',
    'mir.cpp',
  ],
)

idep_mir = declare_dependency(link_with : libmir)

test(
  'mir',
  executable(
    'mir_tests',
    ['graph_test.cpp'],
    dependencies : [idep_mir, dep_gtest],
  ),
  protocol : 'gtest',
)
