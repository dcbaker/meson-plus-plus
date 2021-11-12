// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

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

    // If the true branch is the one we want, move the next and condition to our
    // next and condition, otherwise move the `else` branch to be the main condition, and continue
    const bool & con_v = std::get<std::unique_ptr<Boolean>>(con->condition)->value;
    std::shared_ptr<BasicBlock> next;
    if (con_v) {
        assert(con->if_true != nullptr);
        next = con->if_true;
    } else {
        assert(con->if_false != nullptr);
        next = con->if_false;
    }
    ir->next = next;

    return true;
};

} // namespace MIR::Passes
