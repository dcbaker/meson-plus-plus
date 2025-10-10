// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include "operation.hpp"

#include <string>

namespace MIR {

/// @brief An operation that does not create a target
///
/// These are pure, they don't affect
class Operation {
  public:
    Operation();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

} // namespace MIR
