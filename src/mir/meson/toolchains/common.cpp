// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "common.hpp"
#include "exceptions.hpp"

namespace MIR::Toolchain {

Language from_string(const std::string & str) {
    if (str == "cpp") {
        return Language::CPP;
    }
    // This can happen when an invalid language is passed to the project() function.
    throw Util::Exceptions::MesonException{"No known language \"" + str + "\""};
};

std::string to_string(const Language & l) {
    switch (l) {
        case Language::CPP:
            return "cpp";
        default:
            // TODO: unreachable
            throw Util::Exceptions::MesonException{"This shouldn't be reachable"};
    }
}

} // namespace MIR::Toolchain
