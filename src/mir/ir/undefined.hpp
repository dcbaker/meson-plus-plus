// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include <string>

namespace MIR::IR {

/// @brief A pseudo definition used to make SSA form strict
class Undefined {
  public:
    Undefined();

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;
};

} // namespace MIR::IR
