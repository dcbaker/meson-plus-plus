// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <unordered_map>

#include "lower.hpp"
#include "passes/private.hpp"

namespace MIR {

namespace {

uint64_t lower_impl(BasicBlock & block, State::Persistant & pstate, uint64_t pass = 0) {
    std::unordered_map<std::string, uint32_t> value_number_data{};

    // Print the initial MIR we get from the AST -> MIR conversion
    Passes::printer(block, pass);

    bool progress = true;
    while (progress) {
        pass++;
        progress = Passes::block_walker(
            block,
            {
                [&](BasicBlock & b) { return Passes::flatten(b, pstate); },
                [&](BasicBlock & b) { return Passes::lower_free_functions(b, pstate); },
                Passes::delete_unreachable,
                [&](BasicBlock & b) { return Passes::value_numbering(b, value_number_data); },
                Passes::branch_pruning,
                Passes::join_blocks,
                Passes::fixup_phis,
                Passes::UsageNumbering{},
                Passes::ConstantFolding{},
                Passes::ConstantPropagation{},
                [&](BasicBlock & b) { return Passes::lower_program_objects(b, pstate); },
                [&](BasicBlock & b) { return Passes::lower_string_objects(b, pstate); },
                [&](BasicBlock & b) { return Passes::lower_dependency_objects(b, pstate); },
                [&](BasicBlock & b) { return Passes::printer(b, pass); },
            });
    }

    return ++pass;
}

} // namespace

void lower(BasicBlock & block, State::Persistant & pstate) {

    // Early lowering
    // we can insert compilers and repalce machine calls early, and once, and
    // never worry about them again
    Passes::block_walker(block, {[&](BasicBlock & b) {
                             return Passes::machine_lower(b, pstate.machines) ||
                                    Passes::insert_compilers(b, pstate.toolchains);
                         }});

    // Run the main lowering loop until it cannot lower any more, then do the
    // threaded lowering, which we run across the entire program to lower things
    // like find_program(), Then run the main loop again until we've lowered it
    // all away
    uint64_t pass = lower_impl(block, pstate);
    if (Passes::threaded_lowering(block, pstate)) {
        lower_impl(block, pstate, pass);
    }
}

} // namespace MIR
