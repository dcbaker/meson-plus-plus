// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "identifier.hpp"

#include <sstream>

namespace MIR::IR {

Identifier::Identifier() = default;
Identifier::Identifier(std::string name) : m_name{std::move(name)} {};

std::string Identifier::serialize() const {
    std::stringstream ss;
    ss << "Identifier { "
       << "name = { " << m_name << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
