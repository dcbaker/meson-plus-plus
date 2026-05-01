# SPDX-License-Identifier: Apache-2.0
# Copyright © 2021-2026 Intel Corporation

subdir('util')
subdir('frontend')
subdir('mir')

version_hpp = vcs_tag(input : 'version.hpp.in', output : 'version.hpp')

dep_cli11 = dependency('CLI11')

mesonpp = executable(
  'meson++',
  [
    'main.cpp',
    'options.cpp',
    'tools/compile.cpp',
    'tools/test.cpp',
    'tools/vcs_tag.cpp',
    version_hpp,
    locations_hpp,
  ],
  cpp_args : [
    '-D_GNU_SOURCE', # for getopt
  ],
  dependencies : [idep_frontend, idep_util, idep_mir, dep_cli11],
  install : true,
)
