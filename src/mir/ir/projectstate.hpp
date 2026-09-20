// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <string>

namespace MIR::IR {

/// @brief Object holding the project state.
///
/// This is a sort of psuedo-instruction
class ProjectState {
  public:
    ProjectState();

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;
};

} // namespace MIR::IR
