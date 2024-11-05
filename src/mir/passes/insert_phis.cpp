// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

bool fixup_phis(std::shared_ptr<CFGNode> block) {
    bool progress = false;
    for (auto it = block->block->instructions.begin(); it != block->block->instructions.end();
         ++it) {
        if (std::holds_alternative<Phi>(*it->obj_ptr)) {
            const auto & phi = std::get<Phi>(*it->obj_ptr);
            bool right = false;
            bool left = false;
            for (const auto & p : block->predecessors) {
                for (const Instruction & i : p.lock()->block->instructions) {
                    const auto & var = i.var;
                    if (var.name == it->var.name) {
                        if (var.gvn == phi.left) {
                            left = true;
                            break;
                        }
                        if (var.gvn == phi.right) {
                            right = true;
                            break;
                        }
                    }
                }
                if (left && right) {
                    break;
                }
            }

            if (left ^ right) {
                progress = true;
                auto id =
                    Instruction{Identifier{it->var.name, left ? phi.left : phi.right}, it->var};
                it = block->block->instructions.erase(it);
                it = block->block->instructions.emplace(it, std::move(id));
                continue;
            }

            // While we are walking the instructions in this block, we know that
            // if one side was found, then the other is found that the first
            // found is dead code after the second, so we can ignore it and
            // treat the second one as the truth
            for (auto it2 = block->block->instructions.begin(); it2 != it; ++it2) {
                if (it->var.name == it2->var.name) {
                    left = it2->var.gvn == phi.left;
                    right = it2->var.gvn == phi.right;
                }
            }

            if (left ^ right) {
                progress = true;
                auto id =
                    Instruction{Identifier{it->var.name, left ? phi.left : phi.right}, it->var};
                it = block->block->instructions.erase(it);
                it = block->block->instructions.emplace(it, std::move(id));
            }
        }
    }
    return progress;
}

} // namespace MIR::Passes
