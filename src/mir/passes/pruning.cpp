// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"

namespace MIR::Passes {

bool branch_pruning(BasicBlock * ir) {
    if (ir->condition == nullptr) {
        return false;
    }

    const auto & con = ir->condition;
    if (!std::holds_alternative<std::unique_ptr<Boolean>>(con->condition)) {
        return false;
    }

    // If the true branch is the one we want, move the next and condition to our
    // next and condition, otherwise move the `else` branch to be the main condition, and continue
    const bool & con_v = std::get<std::unique_ptr<Boolean>>(con->condition)->value;
    if (con_v) {
        assert(con->if_true != nullptr);
        ir->next = con->if_true;
        ir->condition = nullptr;
    } else if (con->if_false != nullptr) {
        assert(ir->next == nullptr);
        ir->condition = std::move(con->if_false);
    }

    return true;
};

} // namespace MIR::Passes
