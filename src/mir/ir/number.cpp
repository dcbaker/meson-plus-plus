// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "number.hpp"
#include "helpers.hpp"

#include <sstream>

namespace MIR::IR {

Number::Number(uint64_t v) : value{v} {};

std::string Number::serialize(unsigned indent) const {
    std::stringstream ss{};
    ss << Private::indenter(indent) << "Number { "
       << "value = { " << std::to_string(value) << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
