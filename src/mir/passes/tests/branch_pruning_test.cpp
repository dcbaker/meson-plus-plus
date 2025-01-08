// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(branch_pruning, simple) {
    auto node = lower(R"EOF(
        x = 7
        if true
          x = 8
        endif
        )EOF");
    bool progress = MIR::Passes::graph_walker(node, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(node->block->instructions.size(), 2);
    EXPECT_EQ(node->successors.size(), 1);
    EXPECT_TRUE(std::holds_alternative<MIR::NumberPtr>(node->block->instructions.front()));

    const auto & jump_i = node->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::JumpPtr>(jump_i));
    const auto & target = std::get<MIR::JumpPtr>(jump_i)->target;
    ASSERT_TRUE(node->successors.find(target) != node->successors.end());
    ASSERT_TRUE(target->predecessors.find(node) != target->predecessors.end());
}

TEST(branch_pruning, if_else) {
    auto node = lower(R"EOF(
        x = 7
        if true
          x = 8
        else
          x = 9
        endif
        )EOF");
    bool progress = MIR::Passes::graph_walker(node, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(node->block->instructions.size(), 2);
    EXPECT_EQ(node->successors.size(), 1);
    EXPECT_TRUE(std::holds_alternative<MIR::NumberPtr>(node->block->instructions.front()));

    const auto & jump_i = node->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::JumpPtr>(jump_i));
    const auto & target = std::get<MIR::JumpPtr>(jump_i)->target;
    ASSERT_TRUE(node->successors.find(target) != node->successors.end());
    ASSERT_TRUE(target->predecessors.find(node) != target->predecessors.end());

    ASSERT_EQ(std::get<MIR::NumberPtr>(target->block->instructions.front())->value, 8);
}

TEST(branch_pruning, if_false) {
    auto node = lower(R"EOF(
        x = 7
        if false
          x = 8
        else
          x = 9
          y = 2
        endif
        )EOF");
    bool progress = MIR::Passes::graph_walker(node, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(node->block->instructions.size(), 2);
    EXPECT_EQ(node->successors.size(), 1);
    EXPECT_TRUE(std::holds_alternative<MIR::NumberPtr>(node->block->instructions.front()));

    const auto & jump_i = node->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::JumpPtr>(jump_i));
    const auto & target = std::get<MIR::JumpPtr>(jump_i)->target;
    ASSERT_TRUE(node->successors.find(target) != node->successors.end());
    ASSERT_TRUE(target->predecessors.find(node) != target->predecessors.end());

    ASSERT_EQ(std::get<MIR::NumberPtr>(target->block->instructions.front())->value, 9);
}
