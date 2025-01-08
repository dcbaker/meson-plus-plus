// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

bool fixup_phis(std::shared_ptr<CFGNode> block) {
    bool progress = false;
    for (auto it = block->block->instructions.begin(); it != block->block->instructions.end();
         ++it) {
        if (std::holds_alternative<PhiPtr>(*it)) {
            const auto & phi = std::get<PhiPtr>(*it);
            bool right = false;
            bool left = false;
            for (const auto & p : block->predecessors) {
                for (const Object & i : p.lock()->block->instructions) {
                    const auto & var = std::visit(VariableGetter{}, i);
                    if (var.name == std::visit(VariableGetter{}, *it).name) {
                        if (var.gvn == phi->left) {
                            left = true;
                            break;
                        }
                        if (var.gvn == phi->right) {
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
                const Variable & var = std::visit(VariableGetter{}, *it);
                auto id = std::make_shared<Identifier>(var.name, left ? phi->left : phi->right);
                id->var = var;
                it = block->block->instructions.erase(it);
                it = block->block->instructions.emplace(it, std::move(id));
                continue;
            }

            // While we are walking the instructions in this block, we know that
            // if one side was found, then the other is found that the first
            // found is dead code after the second, so we can ignore it and
            // treat the second one as the truth
            for (auto it2 = block->block->instructions.begin(); it2 != it; ++it2) {
                const MIR::Variable & lvar = std::visit(VariableGetter{}, *it);
                const MIR::Variable & rvar = std::visit(VariableGetter{}, *it2);
                if (lvar.name == rvar.name) {
                    left = rvar.gvn == phi->left;
                    right = rvar.gvn == phi->right;
                }
            }

            if (left ^ right) {
                progress = true;
                const Variable & var = std::visit(VariableGetter{}, *it);
                auto id = std::make_shared<Identifier>(var.name, left ? phi->left : phi->right);
                id->var = var;
                it = block->block->instructions.erase(it);
                it = block->block->instructions.emplace(it, std::move(id));
            }
        }
    }
    return progress;
}

} // namespace MIR::Passes
