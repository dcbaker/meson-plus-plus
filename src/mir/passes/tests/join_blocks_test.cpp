// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "arguments.hpp"
#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(join_blocks, simple) {
    auto irlist = lower(R"EOF(
        x = 7
        if true
          x = 8
        else
          x = 9
        endif
        y = x
        )EOF");

    MIR::Passes::Printer printer{};
    MIR::Passes::graph_walker(irlist, {std::ref(printer)});
    printer.increment();

    bool progress = MIR::Passes::graph_walker(irlist, {
                                                          std::ref(printer),
                                                          MIR::Passes::branch_pruning,
                                                          std::ref(printer),
                                                          MIR::Passes::join_blocks,
                                                            [&printer](std::shared_ptr<MIR::CFGNode> c) {
                                                                printer(c);
                                                                printer.increment();
                                                                return false;
                                                            },
                                                      });
    EXPECT_TRUE(progress);
    EXPECT_TRUE(irlist->successors.empty());
    ASSERT_EQ(irlist->block->instructions.size(), 3);
}

TEST(join_blocks, nested_if) {
    auto irlist = lower(R"EOF(
        x = 7
        if true
          if true
            x = 8
          else
            x = 9
          endif
        endif
        )EOF");
    bool progress = true;
    while (progress) {
        progress = MIR::Passes::graph_walker(
            irlist, {MIR::Passes::branch_pruning, MIR::Passes::join_blocks});
    }
    ASSERT_TRUE(irlist->successors.empty());
    ASSERT_EQ(irlist->block->instructions.size(), 2);
}

TEST(join_blocks, nested_if_elif_else) {
    auto irlist = lower(R"EOF(
        x = 7
        if false
          x = 8
        elif A
          x = 16
        else
          if Q
            y = 7
          else
            y = 9
          endif

          if X
            x = 99
          endif
          x = 9
        endif
        y = x
        z = y
        )EOF");
    bool progress = MIR::Passes::graph_walker(irlist, {
                                                          MIR::Passes::branch_pruning,
                                                          MIR::Passes::join_blocks,
                                                      });
    ASSERT_TRUE(progress);

    // Check that the predecessors of the final block are correct
    const auto & arm = std::get<1>(
        std::get<MIR::Branch>(*irlist->block->instructions.back().obj_ptr).branches.at(0));
    const auto & fin = std::get<MIR::Jump>(*arm->block->instructions.back().obj_ptr).target;

    ASSERT_EQ(fin->block->instructions.size(), 2);
    ASSERT_TRUE(fin->predecessors.count(arm));
    ASSERT_EQ(fin->predecessors.size(), 2);
}
