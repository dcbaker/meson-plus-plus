// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace MIR::IR {

class Message;
class Operation;
class State;
class Target;

using InstructionType = std::variant<std::shared_ptr<Message>, std::shared_ptr<Target>,
                                     std::shared_ptr<Operation>, std::shared_ptr<State>>;

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

using PositionalArguments = std::vector<Instruction>;
using KeywordArguments = std::unordered_map<std::string, Instruction>;

} // namespace MIR::IR
