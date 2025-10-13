// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "boolean.hpp"

#include <sstream>

namespace MIR::IR {

Boolean::Boolean(bool v) : value{v} {};

std::string Boolean::serialize() const {
    std::stringstream ss{};
    ss << "Boolean { "
       << "value = { " << std::to_string(value) << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
