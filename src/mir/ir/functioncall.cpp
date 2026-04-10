// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "functioncall.hpp"
#include "instruction.hpp"
#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

std::string to_string(const PositionalArguments & p_args) {
    std::stringstream ss{};
    ss << "PositionalArguments { ";
    for (const auto & p : p_args) {
        ss << to_string(p);
    }
    ss << " }";

    return ss.str();
}

std::string to_string(const KeywordArguments & k_args) {
    std::stringstream ss{};
    ss << "KeywordArguments { ";
    for (const auto & [k, v] : k_args) {
        ss << to_string(k) << " = { " << to_string(v) << " } ";
    }
    ss << "}";

    return ss.str();
}

std::string to_string(const InstructionType & inst) {
    return std::visit([](auto && i) -> std::string { return i->serialize(); }, inst);
}

FunctionCall::FunctionCall(std::string name) : m_name{name} {};
FunctionCall::FunctionCall(std::string name, PositionalArguments && pos, KeywordArguments && kws)
    : m_name{name}, m_pos{std::move(pos)}, m_kws{std::move(kws)} {};
FunctionCall::FunctionCall(std::string name, InstructionType && ns, PositionalArguments && pos,
                           KeywordArguments && kws)
    : m_name{name}, m_namespace{std::move(ns)}, m_pos{std::move(pos)}, m_kws{std::move(kws)} {};

std::string FunctionCall::serialize() const {
    std::stringstream ss;
    ss << "FunctionCall { "
       << "name = { " << m_name << " } ";
    if (m_namespace) {
        ss << "namespace = { " << to_string(m_namespace.value()) << " } ";
    }
    ss << "positional_arguments = { " << to_string(m_pos) << " } "
       << "keyword_arguments = { " << to_string(m_kws) << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
