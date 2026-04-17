// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include "instruction.hpp"

#include <vector>

namespace MIR::IR {

/// @brief A Ternary object
class Ternary {
  public:
    Ternary(InstructionType condition, InstructionType lhs, InstructionType rhs);

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;

    InstructionType m_cond;
    InstructionType m_lhs;
    InstructionType m_rhs;
};

} // namespace MIR::IR
