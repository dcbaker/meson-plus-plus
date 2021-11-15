// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "lower.hpp"
#include "passes/private.hpp"

namespace MIR {

void lower(BasicBlock * block, State::Persistant & pstate) {
    bool progress = false;
    // clang-format off
    do {
        progress = Passes::block_walker(
            block,
            {
                [&](BasicBlock * b) { return Passes::machine_lower(b, pstate.machines); },
                [&](BasicBlock * b) { return Passes::insert_compilers(b, pstate.toolchains); },
                [&](BasicBlock * b) { return Passes::flatten(b, pstate); },
                [&](BasicBlock * b) { return Passes::lower_free_functions(b, pstate); },
                Passes::branch_pruning,
                Passes::join_blocks,
            }
        );
    } while (progress);
    // clang-format on
}

} // namespace MIR
