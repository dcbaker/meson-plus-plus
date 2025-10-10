// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include "message.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace MIR {

class Operation;
class Target;
class State;

using InstructionType = std::variant<std::shared_ptr<Message>, std::shared_ptr<Target>,
                                     std::shared_ptr<Operation>, std::shared_ptr<State>>;

class Instruction;

using PositionalArguments = std::vector<Instruction>;
using KeywordArguments = std::unordered_map<std::string, Instruction>;

/// @brief A Target of some kind.
///
/// These have inherent side-effects of creating targets
class Target {
  public:
    Target();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

/// @brief An operation that does not create a target
///
/// These are pure, they don't affect
class Operation {
  public:
    Operation();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

/// @brief An operation on the program state
///
/// These have the inherit side effect of modifying the program state
class State {
  public:
    State();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

/// @brief Object holding the project state.
///
/// This is a sort of psuedo-instruction
class ProjectState {
  public:
    ProjectState();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

/// @brief Information about variable storage
class Variable {
  public:
    Variable();

    operator bool() const;

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

/// @brief A basic instruction
class Instruction {
  public:
    Instruction(InstructionType && inst);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    /// @brief The held instruction
    InstructionType instruction;

    /// @brief the storage of this variable
    Variable variable;
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
