// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <string>

namespace MIR {

/// @brief Object holding the project state.
///
/// This is a sort of psuedo-instruction
class ProjectState {
  public:
    ProjectState();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;
};

} // namespace MIR
