// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "number.hpp"

#include <sstream>

namespace MIR::IR {

Number::Number(uint64_t v) : value{v} {};

std::string Number::serialize() const {
    std::stringstream ss{};
    ss << "Number { "
       << "value = { " << std::to_string(value) << " } "
       << " }";
    return ss.str();
}

} // namespace MIR::IR
