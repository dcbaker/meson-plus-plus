// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/linker.hpp"

namespace Meson::Toolchain::Linker::Drivers {

RSPFileSupport Gnu::rsp_support() const {
    return linker.rsp_support();
}

} // namespace HIR::Toolchain::Linker::Drivers
