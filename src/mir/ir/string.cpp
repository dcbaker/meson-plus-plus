// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "string.hpp"

namespace MIR::IR {

String::String(std::string_view v) : m_value{v} {};

std::string String::serialize() const { return "String { " + m_value + " }"; }

} // namespace MIR::IR
