# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

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
    idep_ninja,
  ],
  install : true,
)
