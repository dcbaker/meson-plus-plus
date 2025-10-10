// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "projectstate.hpp"

#include <sstream>

namespace MIR::IR {

ProjectState::ProjectState() = default;

std::string ProjectState::serialize() const { return "ProjectState { }"; }

} // namespace MIR::IR
