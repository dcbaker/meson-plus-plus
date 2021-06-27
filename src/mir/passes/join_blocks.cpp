// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"

namespace MIR::Passes {

bool join_blocks(BasicBlock * block) {
    // If there is a condition we can't join this block to the next one
    if (block->condition.has_value()) {
        return false;
    }

    // If there isn't a next block, then we obviously can't do anything
    if (block->next == nullptr) {
        return false;
    }

    // Move the instructions of the next block into this one, then the condition
    // if neceissry, then make the next block the next->next block.
    block->instructions.splice(block->instructions.end(), block->next->instructions);
    if (block->next->condition.has_value()) {
        block->condition = std::move(block->next->condition);
    }
    block->next = block->next->next;

    return true;
}

} // namespace MIR::Passes
