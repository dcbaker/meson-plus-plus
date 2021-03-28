# SPDX-license-identifier: Apache-2.0
# Copyright © 2021 Dylan Baker

_hir_args = []
foreach h : ['endian.h']
  if cpp.has_header(h)
    _hir_args += '-DHAVE_@0@'.format(h.underscorify().to_upper())
    break
  endif
endforeach

static_library(
  'hir',
  [
    'machines.cpp',
    'toolchains/archivers/gnu.cpp',
    'toolchains/compilers/cpp/gnu.cpp',
    'toolchains/linker_drivers/gnu.cpp',
    'toolchains/linkers/gnu.cpp',
  ],
  cpp_args : _hir_args,
)
