# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

libmesonpp = static_library(
  'meson++',
  [
    'include_directories.hpp',
  ],
)

test(
  'intermediate tests',
  executable(
    'intermediate_tests',
    'intermediate_tests.cpp',
    link_with : libmesonpp,
    dependencies : dep_gtest,
  ),
  protocol : 'gtest',
)