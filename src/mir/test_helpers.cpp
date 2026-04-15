// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "test_helpers.hpp"

namespace MIR::UT {

/// @brief Get an instruction from the CFG by index
/// @param bb the CFG to get the instruction from
/// @param index The index of that instruction, may be either positive or negative
/// @return A const reference to that instruction
const MIR::IR::Instruction & get_ir(const MIR::IR::BasicBlock & bb, int index) {
    if (index >= 0) {
        auto itr = bb.instructions.begin();
        for (int i = 0; i < index; ++i) {
            ++itr;
        }
        return **itr;
    }
    auto itr = bb.instructions.end();
    for (int i = 0; i > index; --i) {
        --itr;
    }
    return **itr;
}

const MIR::IR::Instruction & get_ir(const MIR::IR::CFG & cfg, int index) {
    return get_ir(*cfg.root->block, index);
}

const MIR::IR::Instruction & get_ir(const MIR::IR::Node & root, int index) {
    return get_ir(*root.block, index);
}

} // namespace MIR::UT
