# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker

_meson_args = []
foreach h : ['endian.h']
  if cpp.has_header(h)
    _meson_args += '-DHAVE_@0@'.format(h.underscorify().to_upper())
    break
  endif
endforeach

libmeson_files = []
if dep_pkgconf.found()
  libmeson_files += [
    'pkgconf.cpp',
  ]
endif

libmeson = static_library(
  'meson',
  [
    'machines.cpp',
    'state/state.cpp',
    'toolchains/archivers/gnu.cpp',
    'toolchains/common.cpp',
    'toolchains/compilers/cpp/clang.cpp',
    'toolchains/compilers/cpp/gnu.cpp',
    'toolchains/compilers/cpp/gnulike.cpp',
    'toolchains/detect_archivers.cpp',
    'toolchains/detect_compilers.cpp',
    'toolchains/detect_linkers.cpp',
    'toolchains/linker_drivers/gnu.cpp',
    'toolchains/linkers/gnu.cpp',
    'toolchains/toolchain.cpp',
    'version.cpp',
    libmeson_files,
  ],
  cpp_args : _meson_args,
  dependencies : [idep_util, dep_pkgconf],
)

idep_meson = declare_dependency(
  link_with : libmeson,
  include_directories : include_directories('.'),
  dependencies : [dep_pkgconf],
)

foreach t : ['compiler', 'archiver', 'linker']
  test(
    '@0@ detection'.format(t),
    executable(
      '@0@_detection_test'.format(t),
      'toolchains/detect_@0@s_test.cpp'.format(t),
      link_with : libmeson,
      dependencies : dep_gtest,
    ),
    protocol : 'gtest',
  )
endforeach

test(
  'meson objects',
  executable(
    'meson_object_test',
    'meson_tests.cpp',
    link_with : libmeson,
    dependencies : dep_gtest,
  ),
  protocol : 'gtest',
)
