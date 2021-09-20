// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/archiver.hpp"

namespace MIR::Toolchain::Archiver {

RSPFileSupport Gnu::rsp_support() const { return RSPFileSupport::GCC; };

std::vector<std::string> Gnu::command() const { return _command; };

} // namespace MIR::Toolchain::Archiver
