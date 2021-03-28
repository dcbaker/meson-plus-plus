// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/linkers/gnu.hpp"

namespace HIR::Toolchain::Linker {

RSPFileSupport GnuBFD::rsp_support() const {
    return RSPFileSupport::GCC;
};

} // namespace HIR::Toolchain::Linker
