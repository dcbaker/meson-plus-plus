// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "mir.hpp"

#include <sstream>

namespace MIR {

Target::Target() = default;

std::string Target::serialize() const { return "Target { }"; }

Operation::Operation() = default;

std::string Operation::serialize() const { return "Operation { }"; }

State::State() = default;

std::string State::serialize() const { return "State { }"; }

ProjectState::ProjectState() = default;

std::string ProjectState::serialize() const { return "ProjectState { }"; }

BasicBlock::BasicBlock() = default;

std::string BasicBlock::serialize() const {
    std::stringstream ss{};
    ss << "Basic Block {\n";
    for (auto & i : instructions) {
        ss << "  " << i.serialize() << "\n";
    }
    ss << "}";
    return ss.str();
}

} // namespace MIR
