// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <string>

namespace MIR::IR {

/// @brief A string object
class Identifier {
  public:
    Identifier();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

} // namespace MIR::IR
