// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include "instruction.hpp"

#include <list>
#include <memory>
#include <string>

namespace MIR::IR {

/// @brief A block containing a list of instructions
class BasicBlock {
  public:
    BasicBlock();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    /// @brief The list of instructions
    std::list<std::unique_ptr<Instruction>> instructions;
};

} // namespace MIR::IR
