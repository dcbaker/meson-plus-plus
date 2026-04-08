// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <string>

namespace MIR::IR {

/// @brief A string object
class String {
  public:
    String(std::string_view v);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    std::string m_value;
};

} // namespace MIR::IR
