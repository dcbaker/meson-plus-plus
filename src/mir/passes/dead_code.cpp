// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

bool delete_unreachable(std::shared_ptr<CFGNode> block) {
    // If we see an Message object that is an error, that block will not return,
    // break it's next connection
    std::set<std::shared_ptr<CFGNode>, CFGComparitor> keep;

    for (auto itr = block->block->instructions.begin(); itr != block->block->instructions.end();
         ++itr) {
        if (auto * m = std::get_if<Jump>(itr->obj_ptr.get())) {
            keep.emplace(m->target);
        } else if (auto * m = std::get_if<Branch>(itr->obj_ptr.get())) {
            for (auto && [_, b] : m->branches) {
                keep.emplace(b);
            }
            continue;
        } else if (auto * m = std::get_if<Message>(itr->obj_ptr.get())) {
            if (m->level == MessageLevel::ERROR) {
                bool progress = false;

                while (!block->successors.empty()) {
                    auto b = *block->successors.begin();
                    if (keep.find(b) == keep.end()) {
                        unlink_nodes(block, b);
                        progress = true;
                    }
                }

                if (++itr != block->block->instructions.end()) {
                    // Delete all instructions after this message, they don't actually exists
                    // This may delete additional errors, but we can't be sure
                    // they're not spurious or caused by the first error
                    block->block->instructions.erase(itr, block->block->instructions.end());
                    progress = true;
                }

                return progress;
            }
        }
    }

    return false;
}

} // namespace MIR::Passes
