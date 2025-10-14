// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <string>

#include "instruction.hpp"

namespace MIR::IR {

enum class OperationType {
    // Maps to <object>.<attribute>
    get_attribute,
};

/// @brief An operation that does not create a target
///
/// These are pure, they don't affect

// TODO: should this be InstructionType or Instruction?
class Operation {
  public:
    Operation(Instruction left, OperationType t, Instruction right);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    Instruction left;
    OperationType type;
    Instruction right;
};

} // namespace MIR::IR
