// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include "ir.hpp"

#include <cstdint>

namespace MIR::UT {

/// @brief Get an instruction from the CFG by index
/// @param bb the CFG to get the instruction from
/// @param index The index of that instruction, may be either positive or negative
/// @return A const reference to that instruction
const MIR::IR::Instruction & get_ir(const MIR::IR::CFG & cfg, int64_t index);
const MIR::IR::Instruction & get_ir(const MIR::IR::BasicBlock & root, int64_t index);
const MIR::IR::Instruction & get_ir(const MIR::IR::BasicBlock * root, int64_t index);

template <typename T> constexpr bool holds(const MIR::IR::InstructionType & inst) {
    return std::holds_alternative<std::shared_ptr<T>>(inst);
}

template <typename T> const constexpr T & get(const MIR::IR::InstructionType & inst) {
    return *std::get<std::shared_ptr<T>>(inst);
}

} // namespace MIR::UT
