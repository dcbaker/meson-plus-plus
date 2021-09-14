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
  ],
  dependencies : [
    idep_frontend,
    idep_mir,
  ],
  install : true,
)
