# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Dylan Baker
# Copyright © 2021-2024 Intel Corporation

libutil = static_library(
  'util',
  [
    'log.cpp',
    'process.cpp',
    'utils.cpp',
  ],
)

idep_util = declare_dependency(
  link_with : libutil,
  include_directories : include_directories('.'),
)
