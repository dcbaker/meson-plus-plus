// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "arguments.hpp"
#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(join_blocks, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nelse\n x = 9\nendif\ny = x");
    bool progress = MIR::Passes::graph_walker(irlist, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist->next));
    const auto & next = get_bb(irlist->next);
    ASSERT_EQ(next->block->instructions.size(), 1);

    progress = MIR::Passes::graph_walker(irlist, {MIR::Passes::join_blocks});
    ASSERT_TRUE(progress);
    ASSERT_TRUE(is_empty(irlist->next));
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
        progress = MIR::Passes::graph_walker(irlist, {
                                                         MIR::Passes::branch_pruning,
                                                         MIR::Passes::join_blocks,
                                                     });
    }
    ASSERT_TRUE(std::holds_alternative<std::monostate>(irlist->next));
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
    const auto & con1 = get_con(irlist->next);
    const auto & bb1 = con1->if_true;

    const auto & fin = get_bb(bb1->next);
    ASSERT_EQ(fin->block->instructions.size(), 2);
    ASSERT_TRUE(fin->predecessors.count(bb1));
    ASSERT_EQ(fin->predecessors.size(), 2);
}
