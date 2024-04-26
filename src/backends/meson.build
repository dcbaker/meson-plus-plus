# SPDX-License-Identifier: Apache-2.0
# Copyright © 2021-2024 Intel Corporation

lib_backend = static_library(
  'backend',
  [
    'common/backend.cpp',
    'fir/mir_to_fir.cpp',
    'ninja/ninja.cpp',
  ],
  dependencies : [
    idep_mir,
    idep_util,
  ],
)

idep_backend = declare_dependency(
  link_with : lib_backend,
  include_directories : include_directories('.'),
  dependencies : idep_mir.partial_dependency(includes : true),
)

test(
  'backends',
  executable(
    'backend_tests',
    'backend_tests.cpp',
    dependencies : [idep_backend, dep_gtest],
  ),
  protocol : 'gtest',
)
