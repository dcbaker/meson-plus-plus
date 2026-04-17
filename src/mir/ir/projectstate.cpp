// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "projectstate.hpp"
#include "helpers.hpp"

#include <sstream>

namespace MIR::IR {

ProjectState::ProjectState() = default;

std::string ProjectState::serialize(unsigned indent) const {
    return Private::indenter(indent) + "ProjectState { }";
}

} // namespace MIR::IR
