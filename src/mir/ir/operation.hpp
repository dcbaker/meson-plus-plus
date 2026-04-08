// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <string>

#include "instruction.hpp"

namespace MIR::IR {

/// @brief Operations acting on one source
enum class Operation1SrcType {
    /// @brief logical not
    lnot,

    /// @brief Numerical negation, i.e., -10
    negate,
};

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

    /// @brief Maps x < y
    lt,

    /// @brief Maps x ≤ y
    le,

    /// @brief Maps x == y
    eq,

    /// @brief Maps x ≠ y
    ne,

    /// @brief Maps x ≥ y
    ge,

    /// @brief Maps x > y
    gt,

    /// @brief Maps x not in y
    not_in,

    /// @brief Maps x in y
    in,

    /// @brief Maps x and y
    and_,

    /// @brief Maps x or y
    or_,

    /// @brief Maps x * y
    mul,

    /// @brief Maps x / y
    div,

    /// @brief Maps x % y
    mod,

    /// @brief Maps x + y
    add,

    /// @brief Maps x - y
    sub,
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
