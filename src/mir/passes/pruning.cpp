// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <algorithm>

#include "passes.hpp"

namespace MIR::Passes {

namespace {

bool branch_pruning_impl(std::shared_ptr<CFGNode> node) {
    // If we don't have at least 2 potential exits from this block then we don't
    // have anything to do
    if (node->successors.size() < 2) {
        return false;
    }

    bool progress = false;

    // XXX: this heavily assumes that there is one and only one way to get from
    // one node to a second node. That is not true
    for (auto it = node->block->instructions.begin(); it != node->block->instructions.end(); ++it) {
        if (Jump * j = std::get_if<Jump>(it->obj_ptr.get()); j && j->predicate) {
            if (Boolean * b = std::get_if<Boolean>(j->predicate->obj_ptr.get())) {
                if (b->value) {
                    // If this predicate is true, then we always make this jump.
                    // delete the predicate, erase all of the rest of the instructions
                    // Break all of the other links, and leave
                    j->predicate = std::make_shared<Instruction>();
                    for (auto && s : node->successors) {
                        if (s != j->target) {
                            unlink_nodes(node, s);
                        }
                    }
                    while (it != node->block->instructions.end()) {
                        node->block->instructions.erase(++it);
                    }
                    return true;
                } else {
                    // Otherwise, if the predicate is false, then we can unlink it's
                    // target, and remove the instruction
                    unlink_nodes(node, j->target);
                    it = node->block->instructions.erase(it);
                    progress = true;
                    continue;
                }
            }
        } else if (auto * b = std::get_if<Branch>(it->obj_ptr.get())) {
            assert(!b->branches.empty());

            // If the first branch is true, then we will take that branch of the
            // if/elif/else construct. Throw the rest away and replace the Branch with a Jump
            const auto [con, dest] = b->branches.at(0);
            if (const Boolean * v = std::get_if<Boolean>(con.obj_ptr.get()); v && v->value) {
                for (auto it2 = ++b->branches.begin(); it2 != b->branches.end(); ++it2) {
                    auto t = std::get<1>(*it2);
                    if (t != dest) {
                        unlink_nodes(node, t);
                    }
                }
                it = node->block->instructions.erase(it);
                node->block->instructions.insert(it, Jump{dest});
                progress = true;
            } else {
                // If we find a Branch we can prune any jumps it would make, as well
                // as replacing it with a jump if only one branch is left.
                for (auto it2 = b->branches.begin(); it2 != b->branches.end(); ++it2) {
                    if (auto * con = std::get_if<Boolean>(std::get<0>(*it2).obj_ptr.get());
                        con && !con->value) {
                        unlink_nodes(node, std::get<1>(*it2));
                        it2 = b->branches.erase(it2);
                        progress = true;
                    }
                }

                if (b->branches.size() == 1) {
                    Jump jump{std::get<1>(b->branches.at(0))};
                    it = node->block->instructions.erase(it);
                    node->block->instructions.emplace(it, std::move(jump));
                } else if (b->branches.empty()) {
                    assert(node->successors.empty());
                    it = node->block->instructions.erase(it);
                }
            }
        }
    }

    return progress;
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
