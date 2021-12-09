// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <algorithm>

#include "passes.hpp"

namespace MIR::Passes {

namespace {

/**
 * If a branch has been pruned, we need to determin if there's phis to fix up
 * and do so.
 *
 * We need to look up the phi nodes to see if any branches have been pruned,
 * then we need to fix up any phis that point to pruned branches. There are two
 * cases for this:
 *
 *  1. the first phi for a variable, which is going to look like:
 *      x₄ = ϕ(x₁, x₂)
 *  2. an additional phi node, which looks like:
 *      x₄ = ϕ(x₁, x₂)
 *      x₅ = ϕ(x₃, x₄)
 *
 * The simplest way to handle this is by just resolving the Phi if one of the
 * inputs has been removed. (assuming x₂ has been deleted):
 *      x₄ = x₁
 *      x₅ = ϕ(x₃, x₄)
 *
 * Then we can rely constant folding to determine that x₄ is actually just a x₁,
 * and delete that later.
 */
std::optional<std::unique_ptr<Identifier>> fixup_phi(const std::unique_ptr<Phi> & phi,
                                                     const BasicBlock * to_prune) {
    if (const auto & pruned_value = to_prune->variables.find(phi->var.name);
        pruned_value != to_prune->variables.end()) {
        const auto & num =
            std::visit([](const auto & obj) { return obj->var.version; }, *pruned_value->second);
        const auto & name = phi->var.name;

        // In this case our phi does point to a value that is about to be
        // deleted, so in that case We'll replace this Phi with an Identifier
        // pointing to the not deleted value
        if (phi->left == num || phi->right == num) {
            return std::make_unique<Identifier>(name, phi->right == num ? phi->left : phi->right,
                                                Variable{name, phi->var.version});
        }
    }

    return std::nullopt;
}

bool branch_pruning_impl(BasicBlock * ir) {
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

        auto bb = std::get<std::shared_ptr<BasicBlock>>(current->next).get();

        for (const auto & p : bb->parents) {
            if (visited.count(p) || std::count(todo.begin(), todo.end(), p)) {
                for (auto it = bb->instructions.begin(); it != bb->instructions.end(); ++it) {
                    if (std::holds_alternative<std::unique_ptr<Phi>>(*it)) {
                        const auto & phi = std::get<std::unique_ptr<Phi>>(*it);
                        if (auto v = fixup_phi(phi, p); v.has_value()) {
                            it = bb->instructions.erase(it);
                            bb->instructions.emplace(it, std::move(v.value()));
                        }
                    }
                    p->update_variables();
                }
                bb->parents.erase(p);
            }
        }
    }

    ir->next = next;

    return true;
};

} // namespace

bool branch_pruning(BasicBlock * block) {
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
