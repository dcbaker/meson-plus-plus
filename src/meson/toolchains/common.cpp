// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "common.hpp"

namespace Meson::Toolchain {

Language from_string(const std::string & str) {
    if (str == "cpp") {
        return Language::CPP;
    }
    // This can happen when an invalid language is passed to the project() function.
    throw std::exception{};
};

} // namespace Meson::Toolchain
