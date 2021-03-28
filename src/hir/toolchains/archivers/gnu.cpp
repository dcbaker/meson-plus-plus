// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/archivers/gnu.hpp"

namespace HIR::Toolchain::Archiver {

bool Gnu::accepts_rsp_file() const {
    return true;
};

} // namespace HIR::Toolchain::Archiver
