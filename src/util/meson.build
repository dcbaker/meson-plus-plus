# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker
# Copyright © 2021 Intel Corporation

libutil = static_library(
  'util',
  [
    'log.cpp',
    'process.cpp',
  ],
)

idep_util = declare_dependency(
  link_with : libutil,
  include_directories : include_directories('.'),
)
