# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker

libhir = static_library(
  'hir',

  include_directories : inc_frontend,
)

inc_hir = include_directories('.')

idep_hir = declare_dependency(
  include_directories : inc_hir,
)

test(
  'ast_to_hir_test',
  executable(
    'ast_to_hir_test.cpp',
    dependencies : [idep_frontend, idep_hir, dep_gtest],
  ),
  protocol : 'gtest',
)
