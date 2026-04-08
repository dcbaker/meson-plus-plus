// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include "instruction.hpp"

#include <map>

namespace MIR::IR {

/// @brief A Dict object
class Dict {
  public:
    Dict();
    Dict(std::map<InstructionType, InstructionType> v);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    std::map<InstructionType, InstructionType> m_value;
};

} // namespace MIR::IR
