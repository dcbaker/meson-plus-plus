// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "passes.hpp"

#include <cassert>

namespace MIR::Passes {

namespace {

bool join_blocks_impl(std::shared_ptr<CFGNode> block) {
    // If we don't have exactly one successor we can't join any blocks together
    if (block->successors.size() != 1) {
        return false;
    }

    // If the next block has more than one parent we can't join them yet,
    // otherwise the other parent would end up with a pointer to an empty block
    std::shared_ptr<CFGNode> next = *block->successors.begin();
    if (next->predecessors.size() > 1) {
        return false;
    }

    // Remove the jump block
    // TODO: could be a branch block?
    assert(std::holds_alternative<Jump>(*block->block->instructions.back().obj_ptr));
    block->block->instructions.pop_back();

    // Move the predecessors and successors from the next block to the curren
    // tone
    for (auto && b : next->successors) {
        link_nodes(block, b);
        unlink_nodes(next, b);
    }
    // Move the instructions
    block->block->instructions.splice(block->block->instructions.end(), next->block->instructions);
    unlink_nodes(block, next);

    return true;
}

} // namespace

bool join_blocks(std::shared_ptr<CFGNode> block) {
    bool progress = false;
    bool lprogress;

    // Run this on the same block as long as it's making progress. We do this so
    // that if the new next block can also be pruned we do that with few
    // iterations.
    do {
        lprogress = join_blocks_impl(block);
        progress |= lprogress;
    } while (lprogress);

    return progress;
}

} // namespace MIR::Passes
