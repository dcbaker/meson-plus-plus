// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"

namespace MIR::Passes {

namespace {

bool join_blocks_impl(BasicBlock * block) {
    // If there isn't a next block, then we obviously can't do anything
    if (!std::holds_alternative<std::shared_ptr<BasicBlock>>(block->next)) {
        return false;
    }

    auto & next = std::get<std::shared_ptr<BasicBlock>>(block->next);

    // If the next block has more than one parent we can't join them yet,
    // otherwise the other parent would end up with a pointer to an empty block
    if (next->parents.size() > 1) {
        return false;
    }

    // Move the instructions of the next block into this one, then the condition
    // if neceissry, then make the next block the next->next block.
    block->instructions.splice(block->instructions.end(), next->instructions);
    block->update_variables();
    auto nn = std::move(next->next);
    if (std::holds_alternative<std::shared_ptr<BasicBlock>>(nn)) {
        const auto & b = std::get<std::shared_ptr<BasicBlock>>(nn);
        b->parents.erase(next.get());
        b->parents.emplace(block);
    } else if (std::holds_alternative<std::unique_ptr<Condition>>(nn)) {
        const auto & con = std::get<std::unique_ptr<Condition>>(nn);
        con->if_true->parents.erase(next.get());
        con->if_true->parents.emplace(block);
        con->if_false->parents.erase(next.get());
        con->if_false->parents.emplace(block);
    }
    block->next = std::move(nn);

    return true;
}

} // namespace

bool join_blocks(BasicBlock * block) {
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
