# SPDX-license-identifier: Apache-2.0
# Copyright © 2021-2022 Intel Corporation

lib_fir = static_library(
  'fir',
  [
    'fir/mir_to_fir.cpp',
  ],
  dependencies : [
    idep_mir,
    idep_util,
  ],
)

lib_ninja = static_library(
  'ninja',
  [
    'ninja/ninja.cpp',
  ],
  link_with : [lib_fir],
  dependencies : [
    idep_mir,
    idep_util,
  ],
)

idep_ninja = declare_dependency(
  link_with : lib_ninja,
  include_directories : include_directories('.'),
)
