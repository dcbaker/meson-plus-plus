// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include "instruction.hpp"

#include <string>

namespace MIR::IR {

enum class StateType {
    /// @brief Add project specific compiler arguments
    project_comp_arguments,

    /// @brief Add project specific link arguments
    project_link_arguments,

    /// @brief Add global specific link arguments
    global_comp_arguments,

    /// @brief Add global specific link arguments
    global_link_arguments,
};

/// @brief An operation on the program state
///
/// These have the inherit side effect of modifying the program state
class State {
  public:
    State(StateType s, PositionalArguments p, KeywordArguments k);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    /// @brief What type of state modification should happen
    StateType type;

    PositionalArguments p_args;
    KeywordArguments k_args;
};

} // namespace MIR::IR
