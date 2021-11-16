// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <unordered_map>

#include "lower.hpp"
#include "passes/private.hpp"

namespace MIR {

void lower(BasicBlock * block, State::Persistant & pstate) {
    bool progress =
        Passes::block_walker(block, {[&](BasicBlock * b) {
                                 return Passes::machine_lower(b, pstate.machines) ||
                                        Passes::insert_compilers(block, pstate.toolchains);
                             }});

    std::unordered_map<std::string, uint32_t> value_number_data{};

    // clang-format off
    do {
        value_number_data.clear();

        progress = Passes::block_walker(
            block,
            {
                [&](BasicBlock * b) { return Passes::flatten(b, pstate); },
                [&](BasicBlock * b) { return Passes::lower_free_functions(b, pstate); },
                [&](BasicBlock * b) { return Passes::value_numbering(b, value_number_data); },
                Passes::branch_pruning,
                Passes::join_blocks,
            }
        );
    } while (progress);
    // clang-format on
}

} // namespace MIR
