// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "phi.hpp"

#include <sstream>

namespace MIR::IR {

Phi::Phi(uint64_t left, std::string name, uint64_t right)
    : m_name{name}, m_left{left}, m_right{right} {};

std::string Phi::serialize() const {
    std::stringstream ss;
    ss << "Phi { "
       << "name = { " << m_name << " } "
       << "left = { " << m_left << " } "
       << "right = { " << m_right << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
