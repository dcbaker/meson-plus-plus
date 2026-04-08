// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <string>

#include "instruction.hpp"

namespace MIR::IR {

/// @brief Operations acting on one source
enum class Operation1SrcType {};

class Operation1Src {
  public:
    Operation1Src(InstructionType src, Operation1SrcType t);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    InstructionType left;
    Operation1SrcType type;
};

/// @brief Operations acting on two sources
enum class Operation2SrcType {
    /// @brief Maps to <object>.<attribute>
    get_attribute,

    /// @brief Maps <object>[<index>]
    subscript,
};

/// @brief An operation that does not create a target
///
/// These are pure, they don't affect

class Operation2Src {
  public:
    Operation2Src(InstructionType left, Operation2SrcType t, InstructionType right);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    InstructionType left;
    Operation2SrcType type;
    InstructionType right;
};

} // namespace MIR::IR
