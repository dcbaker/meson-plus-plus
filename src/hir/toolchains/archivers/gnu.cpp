// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/archiver.hpp"

namespace HIR::Toolchain::Archiver {

RSPFileSupport Gnu::rsp_support() const {
    return RSPFileSupport::GCC;
};

} // namespace HIR::Toolchain::Archiver
