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
dep_pkgconf = dependency(
  'libpkgconf++',
  version : '>= 0.1.0',
  required : get_option('pkgconf'),
  fallback : ['libpkgconfpp', 'dep_libpkgconfpp'],
  default_options : ['default_library=static'],
)
if dep_pkgconf.found()
  add_project_arguments('-DHAVE_LIBPKGCONF', language : 'cpp')
else
  warning('Building without libpkgconf support is not recomended, beware.')
endif

subdir('src')
subdir('tests')
