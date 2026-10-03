// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "functioncall.hpp"
#include "helpers.hpp"
#include "instruction.hpp"
#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

using Private::indenter;

std::string serialize(const PositionalArguments & p_args, unsigned indent) {
    std::stringstream ss{};
    const std::string ind = indenter(indent);

    ss << ind << "PositionalArguments { ";
    if (!p_args.empty()) {
        ss << "\n";
        for (const auto & p : p_args) {
            ss << std::visit([&indent](auto && i) { return i->serialize(indent + 1); }, p) << "\n";
        }
        ss << ind;
    }
    ss << "}";

    return ss.str();
}

std::string serialize(const KeywordArguments & k_args, unsigned indent) {
    auto && visitor = [&indent](auto && i) { return i->serialize(indent + 3); };
    const std::string ind = indenter(indent);
    std::stringstream ss{};

    ss << ind << "KeywordArguments { ";
    if (!k_args.empty()) {
        ss << "\n";
        for (const auto & [k, v] : k_args) {
            ss << indenter(indent + 1) << "pair = {\n"
               << indenter(indent + 2) << "key = {\n"
               << std::visit(visitor, k) << "\n"
               << indenter(indent + 2) << "}\n"
               << indenter(indent + 2) << "value = {\n"
               << std::visit(visitor, v) << "\n"
               << indenter(indent + 2) << "}\n"
               << indenter(indent + 1) << "}\n";
        }
        ss << ind;
    }
    ss << "}";

    return ss.str();
}

FunctionCall::FunctionCall(std::string name)
    : m_name{name}, m_namespace{}, m_func_id{}, m_pos{}, m_kws{} {};
FunctionCall::FunctionCall(std::string name, InstructionType && ns)
    : m_name{name}, m_namespace{std::move(ns)}, m_func_id{}, m_pos{}, m_kws{} {};
FunctionCall::FunctionCall(std::string name, InstructionType && ns, FunctionId fid)
    : m_name{name}, m_namespace{std::move(ns)}, m_func_id{fid}, m_pos{}, m_kws{} {};
FunctionCall::FunctionCall(std::string name, std::string ns)
    : m_name{name}, m_namespace{std::make_shared<String>(ns)}, m_func_id{}, m_pos{}, m_kws{} {};
FunctionCall::FunctionCall(std::string name, std::string ns, FunctionId fid)
    : m_name{name}, m_namespace{std::make_shared<String>(ns)}, m_func_id{fid}, m_pos{}, m_kws{} {};
FunctionCall::FunctionCall(std::string name, PositionalArguments && pos, KeywordArguments && kws)
    : m_name{name}, m_func_id{}, m_pos{std::move(pos)}, m_kws{std::move(kws)} {};
FunctionCall::FunctionCall(std::string name, InstructionType && ns, PositionalArguments && pos,
                           KeywordArguments && kws)
    : m_name{name}, m_namespace{std::move(ns)}, m_func_id{}, m_pos{std::move(pos)},
      m_kws{std::move(kws)} {};

std::string FunctionCall::serialize(unsigned indent) const {
    std::stringstream ss;
    const std::string ind = indenter(indent + 1);

    ss << indenter(indent) << "FunctionCall {\n"
       << ind << "name = { " << m_name << " }\n"
       << ind << "func_id = { " << static_cast<int>(m_func_id) << " }\n";
    if (m_namespace) {
        ss << ind << "namespace = {\n"
           << std::visit([&indent](auto && i) { return i->serialize(indent + 2); },
                         m_namespace.value())
           << "\n"
           << ind << "}\n";
    }
    ss << ind << "positional_arguments = {\n"
       << IR::serialize(m_pos, indent + 2) << "\n"
       << ind << "}\n"
       << ind << "keyword_arguments = {\n"
       << IR::serialize(m_kws, indent + 2) << "\n"
       << ind << "}\n"
       << indenter(indent) << "}";
    return ss.str();
}

} // namespace MIR::IR
