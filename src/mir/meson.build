# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker

subdir('meson')

libmir = static_library(
  'mir',
  [
    'ast_to_mir.cpp',
    'lower.cpp',
    'mir.cpp',
    'passes/compilers.cpp',
    'passes/flatten.cpp',
    'passes/free_functions.cpp',
    'passes/join_blocks.cpp',
    'passes/machines.cpp',
    'passes/pruning.cpp',
    'passes/walkers.cpp',
  ],
  include_directories : inc_frontend,
  dependencies : [idep_util, idep_meson],
)

inc_mir = include_directories('.')

idep_mir = declare_dependency(
  link_with : libmir,
  include_directories : inc_mir,
  dependencies : [idep_meson],
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
    dependencies : [idep_frontend, idep_mir, idep_util, dep_gtest],
  ),
  protocol : 'gtest',
)
