// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "instruction.hpp"

#include "message.hpp"
#include "mir.hpp"

#include <sstream>

namespace MIR {

Variable::Variable() = default;

Variable::operator bool() const { return false; }

std::string Variable::serialize() const { return "Variable { }"; }

Instruction::Instruction(InstructionType && inst) : instruction{inst} {};

std::string Instruction::serialize() const {
    const std::string inst = std::visit([](auto && i) { return i->serialize(); }, instruction);
    const std::string var = variable.serialize();
    return "Instruction { instruction = { " + inst + " } variable = { " + var + " } }";
}

} // namespace MIR
