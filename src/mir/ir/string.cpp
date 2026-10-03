// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "string.hpp"
#include "helpers.hpp"

namespace MIR::IR {

String::String(std::string_view v) : m_value{v} {};

std::string String::serialize(unsigned indent) const {
    return Private::indenter(indent) + "String { value = { " + m_value + " } }";
}

} // namespace MIR::IR
