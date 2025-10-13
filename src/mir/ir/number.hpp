// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include "number.hpp"

#include <cstdint>
#include <string>

namespace MIR::IR {

/// @brief A string object
class Number {
  public:
    Number(uint64_t v);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    uint64_t value;
};

} // namespace MIR::IR
