// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "basicblock.hpp"
#include "helpers.hpp"

#include <sstream>

namespace MIR::IR {

using Private::indenter;

BasicBlock::BasicBlock() = default;

std::string BasicBlock::serialize(unsigned indent) const {
    const std::string ind = indenter(indent);

    std::stringstream ss{};
    ss << ind << "Basic Block {\n";
    for (auto & i : instructions) {
        ss << i->serialize(indent + 1) << "\n";
    }
    ss << ind << "}";
    return ss.str();
}

} // namespace MIR::IR
