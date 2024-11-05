// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <algorithm>

#include "passes.hpp"

namespace MIR::Passes {

namespace {

bool branch_pruning_impl(std::shared_ptr<CFGNode> ir) {
    // If we don't have a condition there's nothing to do
    if (!std::holds_alternative<std::unique_ptr<Condition>>(ir->next)) {
        return false;
    }

    // If the condition expression hasn't been reduced to a boolean then there's
    // nothing to do yet.
    auto & con = *std::get<std::unique_ptr<Condition>>(ir->next);
    if (!std::holds_alternative<Boolean>(*con.condition.obj_ptr)) {
        return false;
    }

    // worklist for cleaning up
    std::vector<std::shared_ptr<CFGNode>> todo{};

    // If the true branch is the one we want, move the next and condition to our
    // next and condition, otherwise move the `else` branch to be the main condition, and
    // continue
    const bool & con_v = std::get<Boolean>(*con.condition.obj_ptr).value;
    std::shared_ptr<CFGNode> next;
    if (con_v) {
        assert(con.if_true != nullptr);
        next = con.if_true;
        todo.emplace_back(con.if_false);
    } else {
        assert(con.if_false != nullptr);
        next = con.if_false;
        todo.emplace_back(con.if_true);
    }

    // When we prune this, we need to all remove it from any successor blocks
    // predecessors' so that we dont reference a dangling pointer

    // Blocks that have already been visited
    std::set<std::shared_ptr<CFGNode>, CFGComparitor> visited{};

    // Walk down the CFG of the block we're about to prune until we find a block
    // with predecessors that aren't visited or todo items, that is the convergance point
    // Then remove this path from that block's predecessors.
    while (!todo.empty()) {
        auto current = todo.back();
        todo.pop_back();
        visited.emplace(current);

        // It is possible to put the last block onot the todo stack, just continue on
        if (std::holds_alternative<std::monostate>(current->next)) {
            continue;
        }

        // If we have a Condition then push the True block, then the False
        // block, then loop back through
        if (std::holds_alternative<std::unique_ptr<Condition>>(current->next)) {
            const auto & con = *std::get<std::unique_ptr<Condition>>(current->next);
            todo.emplace_back(con.if_true);
            todo.emplace_back(con.if_false);
            continue;
        }

        auto bb = std::get<std::shared_ptr<CFGNode>>(current->next).get();

        std::set<std::weak_ptr<CFGNode>, CFGComparitor> new_predecessors{};
        for (const auto & p : bb->predecessors) {
            if (!(visited.count(p.lock()) || std::count(todo.begin(), todo.end(), p.lock()))) {
                new_predecessors.emplace(p);
            }
        }
        bb->predecessors = new_predecessors;
    }

    next->predecessors = {ir};
    ir->next = next;

    return true;
};

} // namespace

bool branch_pruning(std::shared_ptr<CFGNode> block) {
    bool progress = false;
    bool lprogress;

    // Run this on the same block as long as it's making progress. We do this so
    // that if the new next block can also be pruned we do that with few
    // iterations.
    do {
        lprogress = branch_pruning_impl(block);
        progress |= lprogress;
    } while (lprogress);

    return progress;
}

} // namespace MIR::Passes
