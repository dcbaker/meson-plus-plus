// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "toolchains/linker.hpp"

namespace MIR::Toolchain::Linker {

RSPFileSupport GnuBFD::rsp_support() const { return RSPFileSupport::GCC; };

std::vector<std::string> GnuBFD::specialize_argument(const Arguments::Argument & arg,
                                                     const fs::path & src_dir,
                                                     const fs::path & build_dir) const {
    switch (arg.type()) {
        case Arguments::Type::LINK:
            return {"-l", arg.value()};
        case Arguments::Type::LINK_SEARCH:
            return {"-L", arg.value()};
        case Arguments::Type::RAW_LINK:
            return {arg.value()};
        case Arguments::Type::DEFINE:
        case Arguments::Type::INCLUDE:
        case Arguments::Type::RAW:
            return {};
        default:
            throw std::exception{}; // should be unreachable
    }
}

} // namespace MIR::Toolchain::Linker
