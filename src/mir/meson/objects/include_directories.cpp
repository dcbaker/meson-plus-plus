// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "objects.hpp"

namespace MIR::Objects {

IncludeDirectories::IncludeDirectories() : directories{}, is_system{false} {};

IncludeDirectories::IncludeDirectories(const std::vector<std::string> & d, const bool & s)
    : directories{d}, is_system{s} {};

std::vector<std::string> IncludeDirectories::as_strings(const Toolchain::Compiler::Compiler & c,
                                                        const State::Persistant & pstate) const {
    std::vector<std::string> args{};

    for (const auto & d : directories) {
        const auto & incs =
            c.include_directories(d, pstate.source_root, pstate.build_root, is_system);
        args.insert(args.end(), incs.begin(), incs.end());
    }

    return args;
}

} // namespace MIR::Objects
