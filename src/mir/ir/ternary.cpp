// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "ternary.hpp"
#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

Ternary::Ternary(InstructionType condition, InstructionType lhs, InstructionType rhs)
    : m_cond{condition}, m_lhs{lhs}, m_rhs{rhs} {};

std::string Ternary::serialize(unsigned indent) const {
    auto && visitor = [](auto && i) -> std::string { return i->serialize(); };

    std::stringstream ss{};
    ss << "Array { "
       << "condition = { " << std::visit(visitor, m_cond) << " } "
       << "lhs = { " << std::visit(visitor, m_lhs) << " } "
       << "hhs = { " << std::visit(visitor, m_rhs) << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
