# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker

_hir_args = []
foreach h : ['endian.h']
  if cpp.has_header(h)
    _hir_args += '-DHAVE_@0@'.format(h.underscorify().to_upper())
    break
  endif
endforeach

libhir = static_library(
  'hir',
  [
    'machines.cpp',
    'toolchains/archivers/gnu.cpp',
    'toolchains/common.cpp',
    'toolchains/compilers/cpp/clang.cpp',
    'toolchains/compilers/cpp/gnu.cpp',
    'toolchains/detect_archivers.cpp',
    'toolchains/detect_compilers.cpp',
    'toolchains/detect_linkers.cpp',
    'toolchains/linker_drivers/gnu.cpp',
    'toolchains/linkers/gnu.cpp',
    'toolchains/toolchain.cpp',
  ],
  cpp_args : _hir_args,
  dependencies : [idep_util],
)

libast_to_hir = static_library(
  'ast_to_hir',
  [
    'ast_to_hir/functions.cpp',
  ],
  link_with : libhir,
  dependencies : idep_frontend,
)

foreach t : ['compiler', 'archiver', 'linker']
  test(
    '@0@ detection'.format(t),
    executable(
      '@0@_detection_test'.format(t),
      'toolchains/detect_@0@s_test.cpp'.format(t),
      link_with : libhir,
      dependencies : dep_gtest,
    ),
    protocol : 'gtest',
  )
endforeach
