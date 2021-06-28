// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/compilers/cpp/cpp.hpp"

namespace MIR::Toolchain::Compiler::CPP {

RSPFileSupport Gnu::rsp_support() const { return RSPFileSupport::GCC; };

} // namespace MIR::Toolchain::Compiler::CPP
