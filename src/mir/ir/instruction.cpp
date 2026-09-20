// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "instruction.hpp"
#include "helpers.hpp"
#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

using Private::indenter;

Variable::Variable() = default;
Variable::Variable(std::string name) : m_name{name}, m_ssa_id{0} {};

Variable::operator bool() const { return !m_name.empty(); }

std::string Variable::serialize(unsigned indent) const {
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

std::string Instruction::serialize(unsigned indent) const {
    std::stringstream ss{};

    ss << indenter(indent) << "Instruction {\n"
       << indenter(indent + 1) << "instruction = {\n"
       << std::visit([&indent](auto && i) { return i->serialize(indent + 2); }, instruction) << "\n"
       << indenter(indent + 1) << "}\n"
       << indenter(indent + 1) << "variable = { " << variable.serialize() << " }\n"
       << indenter(indent) << "}";
    return ss.str();
}

} // namespace MIR::IR
