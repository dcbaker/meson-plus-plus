// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "lower.hpp"
#include "passes.hpp"
#include "state/state.hpp"

namespace MIR {

void lower(BasicBlock * block, const State::Persistant & pstate) {
    bool progress;
    // clang-format off
    do {
        progress = false
            || Passes::machine_lower(block, pstate.machines)
            || Passes::insert_compilers(block, pstate.toolchains)
            || Passes::lower_free_functions(block, pstate)
            || Passes::branch_pruning(block)
            || Passes::join_blocks(block)
            ;
    } while (progress);
    // clang-format on
}

} // namespace MIR
