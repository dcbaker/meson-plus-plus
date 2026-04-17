// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "boolean.hpp"
#include "helpers.hpp"

#include <sstream>

namespace MIR::IR {

Boolean::Boolean(bool v) : value{v} {};

std::string Boolean::serialize(unsigned indent) const {
    const std::string ind = Private::indenter(indent);

    std::stringstream ss{};
    ss << ind << "Boolean { value = { " << std::to_string(value) << " } }";
    return ss.str();
}

} // namespace MIR::IR
