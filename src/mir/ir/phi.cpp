// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "phi.hpp"
#include "helpers.hpp"

#include <sstream>

namespace MIR::IR {

Phi::Phi(uint64_t left, std::string name, uint64_t right)
    : m_name{name}, m_left{left}, m_right{right} {};

std::string Phi::serialize(unsigned indent) const {
    std::stringstream ss;
    const std::string ind1 = Private::indenter(indent);
    const std::string ind2 = Private::indenter(indent + 1);

    ss << ind1 << "Phi {\n"
       << ind2 << "name = { " << m_name << " }\n"
       << ind2 << "left = { " << m_left << " }\n"
       << ind2 << "right = { " << m_right << " }\n"
       << ind1 << "}";
    return ss.str();
}

} // namespace MIR::IR
