// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/linker.hpp"

namespace MIR::Toolchain::Linker {

RSPFileSupport GnuBFD::rsp_support() const { return RSPFileSupport::GCC; };

} // namespace MIR::Toolchain::Linker
