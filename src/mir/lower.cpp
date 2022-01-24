// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <unordered_map>

#include "lower.hpp"
#include "passes/private.hpp"

namespace MIR {

namespace {

void lower_impl(BasicBlock & block, State::Persistant & pstate) {
    std::unordered_map<std::string, uint32_t> value_number_data{};
    Passes::ReplacementTable rt{};
    Passes::LastSeenTable lst{};
    Passes::PropTable pt{};

    bool progress = true;
    while (progress) {
        progress = Passes::block_walker(
            &block,
            {
                [&](BasicBlock * b) { return Passes::flatten(b, pstate); },
                [&](BasicBlock * b) { return Passes::lower_free_functions(b, pstate); },
                [&](BasicBlock * b) { return Passes::value_numbering(b, value_number_data); },
                Passes::branch_pruning,
                Passes::join_blocks,
                Passes::fixup_phis,
                [&](BasicBlock * b) { return Passes::usage_numbering(b, lst); },
                [&](BasicBlock * b) { return Passes::constant_folding(b, rt); },
                [&](BasicBlock * b) { return Passes::constant_propogation(b, pt); },
                [&](BasicBlock * b) { return Passes::lower_program_objects(*b, pstate); },
            });
    }
}

} // namespace

void lower(BasicBlock * block, State::Persistant & pstate) {

    // Early lowering
    // we can insert compilers and repalce machine calls early, and once, and
    // never worry about them again
    Passes::block_walker(block, {[&](BasicBlock * b) {
                             return Passes::machine_lower(b, pstate.machines) ||
                                    Passes::insert_compilers(block, pstate.toolchains);
                         }});

    // Run the main lowering loop until it cannot lower any more, then do the
    // threaded lowering, which we run across the entire program to lower things
    // like find_program(), Then run the main loop again until we've lowered it
    // all away
    lower_impl(*block, pstate);
    Passes::threaded_lowering(block, pstate);
    lower_impl(*block, pstate);
}

} // namespace MIR
