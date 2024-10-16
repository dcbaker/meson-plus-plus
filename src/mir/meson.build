# SPDX-License-Identifier: Apache-2.0
# Copyright © 2021-2024 Intel Corporation

subdir('meson')

libmir = static_library(
  'mir',
  [
    'ast_to_mir.cpp',
    'lower.cpp',
    'mir.cpp',
    'passes/compilers.cpp',
    'passes/constant_folding.cpp',
    'passes/constant_propogation.cpp',
    'passes/custom_target_program_replacement.cpp',
    'passes/dead_code.cpp',
    'passes/dependency_objects.cpp',
    'passes/flatten.cpp',
    'passes/free_functions.cpp',
    'passes/insert_phis.cpp',
    'passes/join_blocks.cpp',
    'passes/machines.cpp',
    'passes/program_objects.cpp',
    'passes/print.cpp',
    'passes/pruning.cpp',
    'passes/string_objects.cpp',
    'passes/threaded.cpp',
    'passes/value_numbering.cpp',
    'passes/walkers.cpp',
    locations_hpp,
  ],
  include_directories: inc_frontend,
  dependencies: [idep_util, idep_meson, dependency('threads')],
)

inc_mir = include_directories('.')

idep_mir = declare_dependency(
  link_with: libmir,
  include_directories: inc_mir,
  dependencies: [idep_meson],
)

test(
  'ast_to_mir_test',
  executable(
    'ast_to_mir_test',
    ['ast_to_mir_test.cpp', locations_hpp],
    dependencies: [idep_frontend, idep_mir, dep_gtest],
  ),
  protocol: 'gtest',
)

test(
  'mir_test',
  executable(
    'mir_test',
    [
      'mir_tests.cpp',
      locations_hpp,
    ],
    dependencies: [idep_frontend, idep_mir, dep_gtest],
  ),
  protocol: 'gtest',
)

test(
  'mir_passes',
  executable(
    'mir_passes_test',
    [
      'passes/tests/branch_pruning_test.cpp',
      'passes/tests/const_folding_test.cpp',
      'passes/tests/constant_propogation_test.cpp',
      'passes/tests/custom_target_program_replacement_test.cpp',
      'passes/tests/dead_code_test.cpp',
      'passes/tests/fixup_phis_test.cpp',
      'passes/tests/flatten_test.cpp',
      'passes/tests/free_functions_test.cpp',
      'passes/tests/insert_compiler_test.cpp',
      'passes/tests/insert_phis_test.cpp',
      'passes/tests/join_blocks_test.cpp',
      'passes/tests/lower_test.cpp',
      'passes/tests/machine_lower_test.cpp',
      'passes/tests/test_utils.cpp',
      'passes/tests/value_numbering_test.cpp',
      'passes/tests/walker_tests.cpp',
      locations_hpp,
    ],
    dependencies: [idep_frontend, idep_mir, idep_util, dep_gtest],
  ),
  protocol: 'gtest',
)
