// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include "target.hpp"

#include <string>

namespace MIR::IR {

/// @brief A Target of some kind.
///
/// These have inherent side-effects of creating targets
class Target {
  public:
    Target();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

} // namespace MIR::IR
