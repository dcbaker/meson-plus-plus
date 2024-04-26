# SPDX-License-Identifier: Apache-2.0
# Copyright © 2021-2024 Intel Corporation

subdir('util')
subdir('frontend')
subdir('mir')
subdir('backends')

version_hpp = vcs_tag(
  input : 'version.hpp.in',
  output : 'version.hpp',
)

mesonpp = executable(
  'meson++',
  [
    'main.cpp',
    'options.cpp',
    'test/test.cpp',
    version_hpp,
    locations_hpp,
  ],
  cpp_args : [
    '-D_GNU_SOURCE',  # for getopt
  ],
  dependencies : [
    idep_frontend,
    idep_mir,
    idep_util,
    idep_backend,
  ],
  install : true,
)
