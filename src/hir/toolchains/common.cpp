// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <cassert>

#include "common.hpp"

namespace HIR::Toolchain {

Language from_string(const std::string & str) {
    if (str == "cpp") {
        return Language::CPP;
    }
    assert(false);  // Should be unreachable
};

} // namespace HIR::Toolchain

