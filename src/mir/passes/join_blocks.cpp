// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "passes.hpp"

namespace MIR::Passes {

namespace {

bool join_blocks_impl(std::shared_ptr<CFGNode> block) {
    // If there isn't a next block, then we obviously can't do anything
    if (!std::holds_alternative<std::shared_ptr<CFGNode>>(block->next)) {
        return false;
    }

    auto & next = std::get<std::shared_ptr<CFGNode>>(block->next);

    // If the next block has more than one parent we can't join them yet,
    // otherwise the other parent would end up with a pointer to an empty block
    if (next->predecessors.size() > 1) {
        return false;
    }

    // Move the instructions of the next block into this one, then the condition
    // if neceissry, then make the next block the next->next block.
    block->block->instructions.splice(block->block->instructions.end(), next->block->instructions);
    auto nn = std::move(next->next);
    if (std::holds_alternative<std::shared_ptr<CFGNode>>(nn)) {
        const auto & b = std::get<std::shared_ptr<CFGNode>>(nn);
        unlink_nodes(next, b);
        link_nodes(block, b);
    } else if (std::holds_alternative<std::unique_ptr<Condition>>(nn)) {
        const auto & con = std::get<std::unique_ptr<Condition>>(nn);
        for (const auto & c : {con->if_true, con->if_false}) {
            unlink_nodes(next, c);
            link_nodes(block, c);
        }
    }
    block->next = std::move(nn);

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
