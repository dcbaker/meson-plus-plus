// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "instruction.hpp"

#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

Variable::Variable() = default;

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

Instruction::Instruction(InstructionType && inst) : instruction{inst} {};

std::string Instruction::serialize() const {
    const std::string inst = std::visit([](auto && i) { return i->serialize(); }, instruction);
    const std::string var = variable.serialize();
    return "Instruction { instruction = { " + inst + " } variable = { " + var + " } }";
}

std::string to_string(PositionalArguments p_args) {
    std::stringstream ss{};
    ss << "PositionalArguments { ";
    for (const auto & p : p_args) {
        ss << p.serialize();
    }
    ss << " }";

    return ss.str();
}

std::string to_string(KeywordArguments k_args) {
    std::stringstream ss{};
    ss << "KeywordArguments { ";
    for (const auto & [k, v] : k_args) {
        ss << k.serialize() << " = { " << v.serialize() << " } ";
    }
    ss << "}";

    return ss.str();
}

} // namespace MIR::IR
