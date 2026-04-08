// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include <cstdint>
#include <string>

namespace MIR::IR {

/// @brief A Phi Node
class Phi {
  public:
    Phi(uint64_t left, std::string name, uint64_t right);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    std::string m_name;
    uint64_t m_left;
    uint64_t m_right;
};

} // namespace MIR::IR
