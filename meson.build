# SPDX-License-Identifier: Apache-2.0
# Copyright © 2021-2026 Intel Corporation

project(
  'meson++',
  ['cpp'],
  version : '0.0.1',
  meson_version : '>= 0.55',
  license : 'Apache-2.0',
  default_options : ['cpp_std=c++17', 'c_std=c99'],
)

cpp = meson.get_compiler('cpp')

if get_option('debug')
  add_project_arguments('-DMESONPP_DEBUG', language : 'cpp')
endif

add_project_arguments(
  cpp.get_supported_arguments(
    '-Werror=switch-enum',
    '-Werror=implicit-fallthrough',
    '-Werror=return-type',
    '-Werror=unused-result',
  ),
  '-D_GNU_SOURCE',
  language : 'cpp',
)

# Older versions of GCC and Clang require linking with special libraries to get
# std::filesystem support
dep_fs = dependency('', required : false)
if cpp.get_id() == 'gcc'
  dep_fs = cpp.find_library('stdc++fs', required : false)
elif cpp.get_id() == 'clang'
  dep_fs = cpp.find_library('c++fs', required : false)
endif

dep_gtest = dependency(
  'gtest_main',
  disabler : true,
  required : get_option('tests'),
  fallback : ['gtest', 'gtest_main_dep'],
)

subdir('src')
