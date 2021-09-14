# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

subdir('util')
subdir('frontend')
subdir('mir')
subdir('backends')

executable(
  'meson++',
  [
    'main.cpp',
    'options.cpp',
  ],
  cpp_args : [
    '-D_GNU_SOURCE',  # for getopt
  ],
  dependencies : [
    idep_frontend,
    idep_mir,
    idep_util,
  ],
  install : true,
)
