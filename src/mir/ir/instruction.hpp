// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace MIR::IR {

class Message;
class Operation1Src;
class Operation2Src;
class Target;
class String;
class Number;
class Identifier;
class Boolean;
class File;
class Phi;
class Undefined;
class Array;
class Dict;
class Ternary;
class FunctionCall;

using InstructionType =
    std::variant<std::shared_ptr<Message>, std::shared_ptr<Target>, std::shared_ptr<Operation1Src>,
                 std::shared_ptr<Operation2Src>, std::shared_ptr<String>,
                 std::shared_ptr<Identifier>, std::shared_ptr<Number>, std::shared_ptr<Boolean>,
                 std::shared_ptr<File>, std::shared_ptr<Phi>, std::shared_ptr<Undefined>,
                 std::shared_ptr<Array>, std::shared_ptr<Dict>, std::shared_ptr<Ternary>,
                 std::shared_ptr<FunctionCall>>;

/// @brief Information about variable storage
class Variable {
  public:
    Variable();
    Variable(std::string name);

    operator bool() const;

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    /// @brief The name the variable is assigned to
    std::string m_name;

    /// @brief The SSA Id of the variable, corresponding to the name of the
    /// variable
    uint64_t m_ssa_id;
};

/// @brief A basic instruction
class Instruction {
  public:
    Instruction(InstructionType && inst);
    Instruction(InstructionType && inst, Variable && var);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    /// @brief The held instruction
    InstructionType instruction;

    /// @brief the storage of this variable
    Variable variable;
};

std::string to_string(const InstructionType & i);

} // namespace MIR::IR
