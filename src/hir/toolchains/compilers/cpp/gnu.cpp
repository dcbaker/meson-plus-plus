// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/compilers/cpp/gnu.hpp"

namespace HIR::Toolchain::Compiler::CPP {

RSPFileSupport Gnu::rsp_support() const {
    return RSPFileSupport::GCC;
};

} // namespace HIR::Toolchain::Compiler::CPP
