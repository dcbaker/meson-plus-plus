// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include "instruction.hpp"

#include <list>
#include <string>

namespace MIR {

/// @brief Object holding the project state.
///
/// This is a sort of psuedo-instruction
class ProjectState {
  public:
    ProjectState();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

/// @brief A block containing a list of instructions
class BasicBlock {
  public:
    BasicBlock();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    /// @brief The list of instructions
    std::list<Instruction> instructions;
};

} // namespace MIR
