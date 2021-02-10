# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Intel Corporation

project(
  'meson++',
  ['cpp'],
  version : '0.0.1',
  meson_version : '>= 0.55',
  license : 'Apache-2.0',
  default_options : ['cpp_std=c++17', 'c_std=c99']
)

cpp = meson.get_compiler('cpp')

dep_gtest = dependency('gtest_main', disabler : true, required : get_option('tests'), fallback : ['gtest', 'gtest_main_dep'])

subdir('src')