// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include "instruction.hpp"

#include <string>

namespace MIR {

/// @brief An operation on the program state
///
/// These have the inherit side effect of modifying the program state
class State {
  public:
    State();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

} // namespace MIR
