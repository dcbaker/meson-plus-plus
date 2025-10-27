// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "toolchains/archiver.hpp"

namespace MIR::Toolchain::Archiver {

RSPFileSupport Gnu::rsp_support() const { return RSPFileSupport::GCC; };

std::vector<std::string> Gnu::command() const { return _command; };

std::vector<std::string> Gnu::always_args() const {
    // TODO: this is wrong for some platforms?
    return {"csrD"};
}

} // namespace MIR::Toolchain::Archiver
