// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <algorithm>

#include "passes.hpp"

namespace MIR::Passes {

bool branch_pruning(BasicBlock * ir) {
    // If we don't have a condition there's nothing to do
    if (!std::holds_alternative<std::unique_ptr<Condition>>(ir->next)) {
        return false;
    }

    // If the condition expression hasn't been reduced to a boolean then there's
    // nothing to do yet.
    auto & con = std::get<std::unique_ptr<Condition>>(ir->next);
    if (!std::holds_alternative<std::unique_ptr<Boolean>>(con->condition)) {
        return false;
    }

    // worklist for cleaning up
    std::vector<BasicBlock *> todo{};

    // If the true branch is the one we want, move the next and condition to our
    // next and condition, otherwise move the `else` branch to be the main condition, and continue
    const bool & con_v = std::get<std::unique_ptr<Boolean>>(con->condition)->value;
    std::shared_ptr<BasicBlock> next;
    if (con_v) {
        assert(con->if_true != nullptr);
        next = con->if_true;
        todo.emplace_back(con->if_false.get());
    } else {
        assert(con->if_false != nullptr);
        next = con->if_false;
        todo.emplace_back(con->if_true.get());
    }

    // When we prune this, we need to all remove it from any successor blocks
    // parents' so that we dont reference a dangling pointer

    // Blocks that have already been visited
    std::set<BasicBlock *> visited{};

    // Walk down the CFG of the block we're about to prune until we find a block
    // with parents that aren't visited or todo items, that is the convergance point
    // Then remove this path from that block's parents.
    while (!todo.empty()) {
        auto * current = todo.back();
        todo.pop_back();
        visited.emplace(current);

        // It is possible to put the last block onot the todo stack, just continue on
        if (std::holds_alternative<std::monostate>(current->next)) {
            continue;
        }

        // If we have a Condition then push the True block, then the False
        // block, then loop back through
        if (std::holds_alternative<std::unique_ptr<Condition>>(current->next)) {
            const auto & con = std::get<std::unique_ptr<Condition>>(current->next);
            todo.emplace_back(con->if_true.get());
            todo.emplace_back(con->if_false.get());
            continue;
        }

        auto bb = std::get<std::shared_ptr<BasicBlock>>(current->next);

        auto parents = bb->parents;
        for (const auto & p : parents) {
            if (visited.count(p)) {
                bb->parents.erase(p);
            } else if (std::count(todo.begin(), todo.end(), p)) {
                bb->parents.erase(p);
            }
        }
    }

    ir->next = next;

    return true;
};

} // namespace MIR::Passes
