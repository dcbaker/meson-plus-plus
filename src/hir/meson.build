# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker

libhir = static_library(
  'hir',
  ['hir.cpp', 'passes.cpp', 'ast_to_hir.cpp'],
  include_directories : inc_frontend,
  dependencies : idep_util,
)

inc_hir = include_directories('.')

idep_hir = declare_dependency(
  link_with : libhir,
  include_directories : inc_hir,
)

test(
  'ast_to_hir_test',
  executable(
    'ast_to_hir_test',
    'ast_to_hir_test.cpp',
    dependencies : [idep_frontend, idep_hir, dep_gtest],
  ),
  protocol : 'gtest',
)
