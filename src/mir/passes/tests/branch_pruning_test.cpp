// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(branch_pruning, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nendif\n");
    bool progress = MIR::Passes::block_walker(&irlist, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist.next));
    const auto & next = get_bb(irlist.next);
    ASSERT_FALSE(is_con(next->next));
    ASSERT_EQ(next->instructions.size(), 1);
}

TEST(branch_pruning, next_block) {
    auto irlist = lower(R"EOF(
        x = 7
        if true
          x = 8
        endif
        y = x
        )EOF");
    bool progress = MIR::Passes::block_walker(&irlist, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);
    ASSERT_TRUE(is_bb(irlist.next));

    const auto & next = get_bb(irlist.next);
    ASSERT_EQ(next->instructions.size(), 1);
    ASSERT_EQ(next->parents.size(), 1);
    ASSERT_TRUE(next->parents.count(&irlist));

    ASSERT_EQ(get_bb(next->next)->parents.count(next.get()), 1);
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
    bool progress = MIR::Passes::block_walker(&irlist, {MIR::Passes::branch_pruning});

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist.next));
    const auto & next = get_bb(irlist.next);
    ASSERT_EQ(next->instructions.size(), 1);

    ASSERT_TRUE(is_bb(next->next));
    ASSERT_TRUE(get_bb(next->next)->instructions.empty());
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
    bool progress = MIR::Passes::block_walker(&irlist, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist.next));
    const auto & next = get_bb(irlist.next);
    ASSERT_EQ(next->instructions.size(), 2);

    const auto & first = std::get<std::shared_ptr<MIR::Number>>(next->instructions.front());
    ASSERT_EQ(first->value, 9);
    ASSERT_EQ(first->var.name, "x");

    const auto & last = std::get<std::shared_ptr<MIR::Number>>(next->instructions.back());
    ASSERT_EQ(last->value, 2);
    ASSERT_EQ(last->var.name, "y");
}
