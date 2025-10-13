// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include "string.hpp"

#include <string>

namespace MIR::IR {

/// @brief A string object
class String {
  public:
    String();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

} // namespace MIR::IR
