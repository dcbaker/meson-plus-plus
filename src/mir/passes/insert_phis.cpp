// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"

namespace MIR::Passes {

namespace {

/// Does this block have only one parent?
inline bool is_strictly_dominated(const BasicBlock * block) { return block->parents.size() == 1; }

/// Is this an assignment
inline auto is_variable = [](const auto & obj) { return obj->var; };

} // namespace

bool insert_phis(BasicBlock * block) {
    bool progress = false;

    // If there is only one path into this block then we don't need to worry
    // about variables, they should already be strictly dominated in the parent
    // blocks.
    if (is_strictly_dominated(block)) {
        return false;
    }

    /**
     * Given a black who's parent is a condition, we konw that it's convergance
     * is by definition a dominance frontier
     *
     * we know a block's immediate dominators, as those are the blocks `parents`
     *
     * I have a couple of ideas
     *
     * 1. walk all of the blocks top down using a block_walker, gather the
     *    possible values of every variable in a block, then walk back through
     *    inserting phi nodes from those values.
     * 2. For each block that is not strictly dominated, walk backwards up the
     *    graph until we find all possible versions of a variable. we could keep
     *    a cheat sheet of these values to speed this up
     */

    // A list of new instructions (phis) to prepend to the instruction list
    std::list<Object> phis{};

    for (const auto & obj : block->instructions) {
        if (std::visit(is_variable, obj)) {
        }
    }

    return progress;
}

} // namespace MIR::Passes
