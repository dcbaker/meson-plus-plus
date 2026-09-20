// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include "instruction.hpp"

#include <vector>

namespace MIR::IR {

/// @brief An Array object
class Array {
  public:
    Array();
    Array(std::vector<InstructionType> v);

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;

    std::vector<InstructionType> m_value;
};

} // namespace MIR::IR
