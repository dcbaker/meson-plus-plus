// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(branch_pruning, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nendif\n");
    bool progress = MIR::Passes::graph_walker(irlist, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist->next));
    const auto & next = get_bb(irlist->next);
    ASSERT_FALSE(is_con(next->next));
    ASSERT_EQ(next->block->instructions.size(), 1);
}

TEST(branch_pruning, next_block) {
    auto irlist = lower(R"EOF(
        x = 7
        if true
          x = 8
        endif
        y = x
        )EOF");
    bool progress = MIR::Passes::graph_walker(irlist, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    ASSERT_TRUE(is_bb(irlist->next));

    const auto & next = get_bb(irlist->next);
    ASSERT_EQ(next->block->instructions.size(), 1);
    ASSERT_EQ(next->predecessors.size(), 1);
    ASSERT_TRUE(next->predecessors.count(irlist));

    ASSERT_EQ(get_bb(next->next)->predecessors.count(next), 1);
}

TEST(branch_pruning, if_else) {
    auto irlist = lower(R"EOF(
        x = 7
        if true
          x = 8
        else
          x = 9
        endif
        )EOF");
    bool progress = MIR::Passes::graph_walker(irlist, {MIR::Passes::branch_pruning});

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist->next));
    const auto & next = get_bb(irlist->next);
    ASSERT_EQ(next->block->instructions.size(), 1);

    ASSERT_TRUE(is_bb(next->next));
    ASSERT_TRUE(get_bb(next->next)->block->instructions.empty());
}

TEST(branch_pruning, if_false) {
    auto irlist = lower(R"EOF(
        x = 7
        if false
          x = 8
        else
          x = 9
          y = 2
        endif
        )EOF");
    bool progress = MIR::Passes::graph_walker(irlist, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist->next));
    const auto & next = get_bb(irlist->next);
    ASSERT_EQ(next->block->instructions.size(), 2);

    const auto & first = std::get<MIR::Number>(*next->block->instructions.front().obj_ptr);
    ASSERT_EQ(first.value, 9);
    ASSERT_EQ(next->block->instructions.front().var.name, "x");

    const auto & last = std::get<MIR::Number>(*next->block->instructions.back().obj_ptr);
    ASSERT_EQ(last.value, 2);
    ASSERT_EQ(next->block->instructions.back().var.name, "y");
}
