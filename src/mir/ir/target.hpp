// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <string>

namespace MIR::IR {

/// @brief A Target of some kind.
///
/// These have inherent side-effects of creating targets
class Target {
  public:
    Target();

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;
};

} // namespace MIR::IR
