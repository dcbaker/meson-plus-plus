# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker

libmir = static_library(
  'mir',
  ['mir.cpp', 'passes.cpp', 'ast_to_mir.cpp'],
  include_directories : inc_frontend,
  dependencies : idep_util,
)

inc_mir = include_directories('.')

idep_mir = declare_dependency(
  link_with : libmir,
  include_directories : inc_mir,
)

test(
  'ast_to_mir_test',
  executable(
    'ast_to_mir_test',
    'ast_to_mir_test.cpp',
    dependencies : [idep_frontend, idep_mir, dep_gtest],
  ),
  protocol : 'gtest',
)

test(
  'mir_passes',
  executable(
    'mir_passes_test',
    'passes_test.cpp',
    dependencies : [idep_frontend, idep_mir, dep_gtest],
  ),
  protocol : 'gtest',
)
