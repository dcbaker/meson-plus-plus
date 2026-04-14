// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "dict.hpp"
#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

Dict::Dict() {};
Dict::Dict(std::map<InstructionType, InstructionType> v) : m_value{std::move(v)} {};

std::string Dict::serialize() const {
    auto && visitor = [](auto && i) -> std::string { return i->serialize(); };

    std::stringstream ss{};
    ss << "Dict { value = { ";

    for (auto && [k, v] : m_value) {
        ss << "pair = { "
           << "key = { " << std::visit(visitor, k) << " } "
           << "value = { " << std::visit(visitor, v) << " } "
           << "} ";
    }

    ss << "} }";
    return ss.str();
}

} // namespace MIR::IR
