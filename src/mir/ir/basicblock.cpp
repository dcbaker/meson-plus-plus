// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "basicblock.hpp"

#include <sstream>

namespace MIR {

BasicBlock::BasicBlock() = default;

std::string BasicBlock::serialize() const {
    std::stringstream ss{};
    ss << "Basic Block {\n";
    for (auto & i : instructions) {
        ss << "  " << i.serialize() << "\n";
    }
    ss << "}";
    return ss.str();
}

} // namespace MIR
