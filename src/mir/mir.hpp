// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <list>
#include <memory>
#include <variant>
#include <vector>

/// @brief A printed message of some kind
class Message {
  public:
    Message();
};

/// @brief A Target of some kind.
///
/// These have inherent side-effects of creating targets
class Target {
  public:
    Target();
};

/// @brief An operation that does not create a target
///
/// These are pure, they don't affect
class Operation {
  public:
    Operation();
};

/// @brief An operation on the program state
///
/// These have the inherit side effect of modifying the program state
class State {
  public:
    State();
};

/// @brief Object holding the project state.
///
/// This is a sort of psuedo-instruction
class ProjectState {
  public:
    ProjectState();
};

using InstructionType = std::variant<Message, Target, Operation, State>;

class Variable {
  public:
    Variable();

    operator bool() const;
};

/// @brief A basic instruction
class Instruction {
  public:
    Instruction(InstructionType && inst);

    /// @brief The held instruction
    InstructionType instruction;
    Variable variable;
};

/// @brief A block containing a list of instructions
class BasicBlock {
  public:
    BasicBlock();

    /// @brief The list of instructions
    std::list<Instruction> instructions;
};

/// @brief A single node the Control Flow Graph
class Node {
  public:
    Node();

    /// @brief Possible entries to this node.
    std::vector<std::shared_ptr<Node>> parents;

    /// @brief The possible exits from this node
    std::vector<std::shared_ptr<Node>> children;
};

/// @brief The representation of the Control Flow Graph
class CFG {
  public:
    CFG(std::shared_ptr<Node> r);

    std::shared_ptr<Node> root;
};
