// SPDX-License-Identifier: Apache-2.0
// Copyright © 2022-2025 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"

#include "test_utils.hpp"

TEST(unreachable_code, clear_dead_instructions) {
    auto irlist = lower(R"EOF(
        message('bar')
        error('should be dead')
        warning('should be deleted')
        )EOF");

    MIR::State::Persistant pstate = make_pstate();

    MIR::Passes::graph_walker(
        irlist,
        {
            [&pstate](std::shared_ptr<MIR::CFGNode> b) {
                return MIR::Passes::instruction_walker(*b, {[&pstate](const MIR::Object & inst) {
                    return MIR::Passes::lower_free_functions(inst, pstate);
                }});
            },
            MIR::Passes::delete_unreachable,
        });

    ASSERT_EQ(irlist->block->instructions.size(), 2);

    const auto & msg_obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::MessagePtr>(msg_obj));
    const auto & msg = std::get<MIR::MessagePtr>(msg_obj);
    ASSERT_EQ(msg->level, MIR::MessageLevel::MESSAGE);

    const auto & err_obj = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::MessagePtr>(err_obj));
    const auto & err = std::get<MIR::MessagePtr>(err_obj);
    ASSERT_EQ(err->level, MIR::MessageLevel::ERROR);
}

TEST(unreachable_code, no_jump_after_error) {
    auto irlist = lower(R"EOF(
        if host_machine.system() == 'aix'
            error('foo')
        else
            x = 10
        endif
        message(x)
        )EOF");

    MIR::State::Persistant pstate = make_pstate();
    MIR::Passes::Printer printer{};
    MIR::Passes::graph_walker(irlist, {std::ref(printer)});
    printer.increment();

    MIR::Passes::graph_walker(
        irlist, {
                    [&](std::shared_ptr<MIR::CFGNode> b) {
                        return MIR::Passes::instruction_walker(
                            *b, {[&](const MIR::Object & obj) {
                                     return MIR::Passes::machine_lower(obj, pstate.machines);
                                 },
                                 [&pstate](const MIR::Object & inst) {
                                     return MIR::Passes::lower_free_functions(inst, pstate);
                                 }});
                    },
                    MIR::Passes::delete_unreachable,
                    std::ref(printer),
                });

    EXPECT_EQ(irlist->successors.size(), 2);

    const auto & branch = std::get<MIR::BranchPtr>(irlist->block->instructions.back());
    const auto & dead_arm = std::get<1>(branch->branches.at(0));
    EXPECT_TRUE(dead_arm->successors.empty());

    const auto & live_arm = std::get<1>(branch->branches.at(1));
    const auto & tail = std::get<MIR::JumpPtr>(live_arm->block->instructions.back())->target;

    EXPECT_EQ(tail->predecessors.size(), 1);
    ASSERT_NE(tail->predecessors.find(live_arm), tail->predecessors.end());
}
