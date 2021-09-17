# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

lib_ninja = static_library(
  'ninja',
  [
    'ninja.cpp',
  ],
  dependencies : [
    idep_mir,
    idep_util,
  ],
)

idep_ninja = declare_dependency(
  link_with : lib_ninja,
  include_directories : include_directories('..'),
)
