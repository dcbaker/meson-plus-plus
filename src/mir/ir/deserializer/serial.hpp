// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <stdexcept>
#include <string>

namespace MIR::IR::Serial {

/// @brief what kind of instruction are we encoding?
enum class InstructionType {
    message,
    operation,
    state,
    target,
    string,
    number,
    identifier,
    boolean,
};

InstructionType inst_from_str(std::string_view s) {
    if (s == "Message") {
        return InstructionType::message;
    }
    if (s == "Operation") {
        return InstructionType::operation;
    }
    if (s == "State") {
        return InstructionType::state;
    }
    if (s == "Target") {
        return InstructionType::target;
    }
    if (s == "String") {
        return InstructionType::string;
    }
    if (s == "Number") {
        return InstructionType::number;
    }
    if (s == "Identifier") {
        return InstructionType::identifier;
    }
    if (s == "Boolean") {
        return InstructionType::boolean;
    }
    throw std::runtime_error("Cannot convert string: " + std::string{s} + "to an instruction type");
}

} // namespace MIR::IR::Serial
