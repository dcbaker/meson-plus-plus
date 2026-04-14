// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "instruction.hpp"

#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

Variable::Variable() = default;
Variable::Variable(std::string name) : m_name{name}, m_ssa_id{0} {};

Variable::operator bool() const { return !m_name.empty(); }

std::string Variable::serialize() const {
    std::stringstream ss;
    ss << "Variable = { ";
    if (!m_name.empty()) {
        ss << "name = { " << m_name << " } "
           << "ssa_id = { " << m_ssa_id << " } ";
    }
    ss << "}";
    return ss.str();
}

Instruction::Instruction(InstructionType && inst)
    : instruction{std::move(inst)}, m_is_block_condition{false} {};
Instruction::Instruction(InstructionType && inst, Variable && var)
    : instruction{std::move(inst)}, variable{std::move(var)}, m_is_block_condition{false} {};

std::string Instruction::serialize() const {
    const std::string inst = to_string(instruction);
    const std::string var = variable.serialize();
    return "Instruction { instruction = { " + inst + " } variable = { " + var + " } }";
}

} // namespace MIR::IR
