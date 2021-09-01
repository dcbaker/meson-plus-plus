// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "lower.hpp"
#include "passes.hpp"
#include "state/state.hpp"

namespace MIR {

void lower(BasicBlock * block, const State::Persistant & pstate) {
    bool progress;
    do {
        progress = false;
        progress |= Passes::machine_lower(block, pstate.machines);
        // progress |= Passes::insert_compilers(block, pstate.toolchains);
        progress |= Passes::lower_free_functions(block, pstate);
        progress |= Passes::branch_pruning(block);
        progress |= Passes::join_blocks(block);
    } while (progress);
}

} // namespace MIR
