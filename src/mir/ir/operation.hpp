// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <string>

#include "instruction.hpp"

namespace MIR::IR {

enum class OperationType {
    /// @brief Maps to <object>.<attribute>
    get_attribute,

    /// @brief Maps <object>[<index>]
    subscript,
};

/// @brief An operation that does not create a target
///
/// These are pure, they don't affect

class Operation {
  public:
    Operation(InstructionType left, OperationType t, InstructionType right);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    InstructionType left;
    OperationType type;
    InstructionType right;
};

} // namespace MIR::IR
