// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "mir.hpp"

Message::Message() = default;

Target::Target() = default;

Operation::Operation() = default;

State::State() = default;

ProjectState::ProjectState() = default;

Variable::Variable() = default;

Variable::operator bool() const { return false; }

Instruction::Instruction(InstructionType && inst) : instruction{inst} {};

BasicBlock::BasicBlock() = default;

Node::Node() = default;

CFG::CFG(std::shared_ptr<Node> r) : root{r} {};
