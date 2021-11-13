// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"

namespace MIR::Passes {

bool join_blocks(BasicBlock * block) {
    // If there isn't a next block, then we obviously can't do anything
    if (!std::holds_alternative<std::shared_ptr<BasicBlock>>(block->next)) {
        return false;
    }

    auto & next = std::get<std::shared_ptr<BasicBlock>>(block->next);

    // Move the instructions of the next block into this one, then the condition
    // if neceissry, then make the next block the next->next block.
    block->instructions.splice(block->instructions.end(), next->instructions);
    auto nn = std::move(next->next);
    block->next = std::move(nn);

    return true;
}

} // namespace MIR::Passes
