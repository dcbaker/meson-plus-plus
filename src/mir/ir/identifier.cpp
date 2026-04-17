// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "identifier.hpp"
#include "helpers.hpp"

#include <sstream>

namespace MIR::IR {

Identifier::Identifier() = default;
Identifier::Identifier(std::string name) : m_name{std::move(name)} {};

std::string Identifier::serialize(unsigned indent) const {
    std::stringstream ss;
    ss << Private::indenter(indent) << "Identifier { "
       << "name = { " << m_name << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
