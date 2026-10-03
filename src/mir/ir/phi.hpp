// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include <string>

#include "instruction.hpp"

namespace MIR::IR {

/// @brief A Phi Node
class Phi {
  public:
    Phi(Instruction * left, std::string name, Instruction * right);

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;

    std::string m_name;
    Instruction * m_left;
    Instruction * m_right;
};

} // namespace MIR::IR
