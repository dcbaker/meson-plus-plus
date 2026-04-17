// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "dict.hpp"
#include "helpers.hpp"
#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

using Private::indenter;

Dict::Dict() {};
Dict::Dict(std::map<InstructionType, InstructionType> v) : m_value{std::move(v)} {};

std::string Dict::serialize(unsigned indent) const {
    auto && visitor = [&](auto && i) -> std::string { return i->serialize(); };

    std::stringstream ss{};
    ss << indenter(indent) << "Dict {\n" << indenter(indent + 1) << "value = {\n";

    for (auto && [k, v] : m_value) {
        ss << indenter(indent + 2) << "pair = {\n"
           << indenter(indent + 3) << "key = { " << std::visit(visitor, k) << " }\n"
           << indenter(indent + 3) << "value = { " << std::visit(visitor, v) << " }\n"
           << indenter(indent + 2) << "} ";
    }

    ss << indenter(indent + 1) << "}" << indenter(indent) << "}";
    return ss.str();
}

} // namespace MIR::IR
