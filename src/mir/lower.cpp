// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <unordered_map>

#include "lower.hpp"
#include "passes/private.hpp"

namespace MIR {

namespace {

// Early lowering
//
// Some passes just only need to be run once for the whole program,
// lowering `*_machine`, and doing our global value numbering and phi
// insertion pass
// TODO: compilers may need to be run again if `add_language` is called
void early(std::shared_ptr<MIR::CFGNode> block, State::Persistant & pstate,
           Passes::Printer & printer) {
    Passes::graph_walker(
        block, {
                   [&](std::shared_ptr<CFGNode> b) {
                       return Passes::instruction_walker(
                           *b,
                           {
                               Passes::custom_target_program_replacement,
                           },
                           {
                               [&pstate](const Object & obj) {
                                   return Passes::insert_compilers(obj, pstate.toolchains);
                               },
                               [&pstate](const Object & obj) {
                                   return Passes::machine_lower(obj, pstate.machines);
                               },
                           });
                   },
                   Passes::GlobalValueNumbering{},
                   std::ref(printer),
               });
}

void main(std::shared_ptr<MIR::CFGNode> block, State::Persistant & pstate,
          Passes::Printer & printer) {
    const std::vector<MIR::Passes::BlockWalkerCb> main_loop{
        [&](std::shared_ptr<CFGNode> b) {
            return Passes::instruction_walker(
                *b, {
                        Passes::disable,
                        Passes::flatten,
                        [&pstate](const Object & i) {
                            return Passes::lower_free_functions(i, pstate);
                        },
                        [&pstate](const Object & i) {
                            return Passes::lower_program_objects(i, pstate);
                        },
                        [&pstate](const Object & i) {
                            return Passes::lower_string_objects(i, pstate);
                        },
                        [&pstate](const Object & i) {
                            return Passes::lower_dependency_objects(i, pstate);
                        },
                    });
        },
        Passes::delete_unreachable,
        Passes::ConstantFolding{},
        Passes::ConstantPropagation{},
        Passes::branch_pruning,
        Passes::join_blocks,
        Passes::fixup_phis,
        std::ref(printer),
    };

    bool progress = false;
    do {
        printer.increment();
        progress = Passes::graph_walker(block, std::ref(main_loop));
    } while (progress);

    // Run the main lowering loop until it cannot lower any more, then do the
    // threaded lowering, which we run across the entire program to lower things
    // like find_program(), Then run the main loop again until we've lowered it
    // all away
    if (Passes::threaded_lowering(block, pstate)) {
        do {
            printer.increment();
            progress = Passes::graph_walker(block, std::ref(main_loop));
        } while (progress);
    }
}

void late(std::shared_ptr<MIR::CFGNode> block, State::Persistant & pstate,
          Passes::Printer & printer) {
    const std::vector<MIR::Passes::BlockWalkerCb> loop{
        MIR::Passes::combine_add_arguments,
        std::ref(printer),
    };

    printer.increment();
    Passes::graph_walker(block, std::ref(loop));
}

} // namespace

void lower(std::shared_ptr<CFGNode> block, State::Persistant & pstate) {
    // Print the initial MIR we get from the AST -> MIR conversion
    Passes::Printer printer{};
    printer(block);
    printer.increment();

    early(block, pstate, printer);
    main(block, pstate, printer);
    late(block, pstate, printer);
}

} // namespace MIR
