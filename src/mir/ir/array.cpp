// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "array.hpp"
#include "instructions.hpp"

#include <sstream>

namespace MIR::IR {

Array::Array() {};
Array::Array(std::vector<InstructionType> v) : m_value{std::move(v)} {};

std::string Array::serialize() const {
    std::stringstream ss{};
    ss << "Array { value = { ";

    for (auto && v : m_value) {
        ss << std::visit([](auto && i) -> std::string { return i->serialize(); }, v) << " ";
    }

    ss << "} }";
    return ss.str();
}

} // namespace MIR::IR
