// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "array.hpp"
#include "instructions.hpp"

#include "helpers.hpp"

#include <sstream>

namespace MIR::IR {

using Private::indenter;

Array::Array() {};
Array::Array(std::vector<InstructionType> v) : m_value{std::move(v)} {};

std::string Array::serialize(unsigned indent) const {
    std::stringstream ss{};
    const std::string ind = indenter(indent);

    ss << ind << "Array {\n" << indenter(indent + 1) << "value = {\n";

    for (auto && v : m_value) {
        ss << std::visit([&indent](auto && i) -> std::string { return i->serialize(indent + 2); },
                         v)
           << "\n";
    }

    ss << indenter(indent + 1) << "}\n" << ind << "}";
    return ss.str();
}

} // namespace MIR::IR
