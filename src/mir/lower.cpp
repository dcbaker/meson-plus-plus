// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <unordered_map>

#include "lower.hpp"
#include "passes/private.hpp"

namespace MIR {

namespace {

uint64_t lower_impl(BasicBlock & block, State::Persistant & pstate, const uint32_t pass = 0) {
    // Print the initial MIR we get from the AST -> MIR conversion
    Passes::Printer printer{pass};
    printer(block);

    bool progress = true;
    while (progress) {
        printer.increment();
        progress = Passes::block_walker(
            block, {
                       [&](BasicBlock & b) { return Passes::flatten(b, pstate); },
                       [&](BasicBlock & b) { return Passes::lower_free_functions(b, pstate); },
                       Passes::delete_unreachable,
                       Passes::branch_pruning,
                       Passes::join_blocks,
                       Passes::fixup_phis,
                       Passes::ConstantFolding{},
                       Passes::ConstantPropagation{},
                       [&](BasicBlock & b) { return Passes::lower_program_objects(b, pstate); },
                       [&](BasicBlock & b) { return Passes::lower_string_objects(b, pstate); },
                       [&](BasicBlock & b) { return Passes::lower_dependency_objects(b, pstate); },
                       std::ref(printer),
                   });
    }

    return printer.pass;
}

} // namespace

void lower(BasicBlock & block, State::Persistant & pstate) {

    // Early lowering
    //
    // Some passes just only need to be run once for the whole program,
    // lowering `*_machine`, and doing our global value numbering and phi
    // insertion pass
    // TODO: compilers may need to be run again if `add_language` is called
    Passes::block_walker(block, {
                                    [&](BasicBlock & b) {
                                        return Passes::machine_lower(b, pstate.machines) ||
                                               Passes::insert_compilers(b, pstate.toolchains);
                                    },
                                    Passes::custom_target_program_replacement,
                                    Passes::GlobalValueNumbering{},
                                });

    // Run the main lowering loop until it cannot lower any more, then do the
    // threaded lowering, which we run across the entire program to lower things
    // like find_program(), Then run the main loop again until we've lowered it
    // all away
    const uint64_t pass = lower_impl(block, pstate);
    if (Passes::threaded_lowering(block, pstate)) {
        lower_impl(block, pstate, pass);
    }
}

} // namespace MIR
