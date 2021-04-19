# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker

libir = static_library(
  'ir',

  include_directories : inc_frontend,
)

inc_ir = include_directories('.')

idep_ir = declare_dependency(
  include_directories : inc_ir,
)

test(
  'ast_to_ir_test',
  executable(
    'ast_to_ir_test.cpp',
    dependencies : [idep_frontend, idep_ir, dep_gtest],
  ),
  protocol : 'gtest',
)
