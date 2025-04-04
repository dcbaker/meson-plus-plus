// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <gtest/gtest.h>

#include "exceptions.hpp"
#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(insert_phi, simple) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        else
            x = 10
        endif
        )EOF");

    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & arm1 =
        std::get<1>(std::get<MIR::BranchPtr>(irlist->block->instructions.back())->branches.at(0));
    const auto & fin = std::get<MIR::JumpPtr>(arm1->block->instructions.back())->target;
    ASSERT_EQ(fin->block->instructions.size(), 1);

    const MIR::Object & instr = fin->block->instructions.front();
    const MIR::Variable & var = std::visit(MIR::VariableGetter{}, instr);
    EXPECT_EQ(var.name, "x");
    EXPECT_EQ(var.gvn, 3); // because value_numbering will run again

    ASSERT_TRUE(std::holds_alternative<MIR::PhiPtr>(instr));
    const auto & phi = std::get<MIR::PhiPtr>(instr);
    EXPECT_EQ(phi->left, 1);
    EXPECT_EQ(phi->right, 2);
}

TEST(insert_phi, three_branches) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        elif 7
            x = 11
        else
            x = 10
        endif
        )EOF");
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & arm1 =
        std::get<1>(std::get<MIR::BranchPtr>(irlist->block->instructions.front())->branches.at(0));
    const auto & fin = std::get<MIR::JumpPtr>(arm1->block->instructions.back())->target;

    ASSERT_EQ(fin->block->instructions.size(), 2);

    auto it = fin->block->instructions.begin();
    {
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, *it);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 4);
    }

    ASSERT_TRUE(std::holds_alternative<MIR::PhiPtr>(*it));
    const auto & phi = std::get<MIR::PhiPtr>(*it);
    EXPECT_EQ(phi->left, 1);
    EXPECT_EQ(phi->right, 2);

    it++;

    {
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, *it);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 5);
    }

    ASSERT_TRUE(std::holds_alternative<MIR::PhiPtr>(*it));
    const auto & phi2 = std::get<MIR::PhiPtr>(*it);
    EXPECT_EQ(phi2->left, 4);
    ASSERT_EQ(phi2->right, 3);
}

TEST(insert_phi, nested_branches) {
    auto irlist = lower(R"EOF(
        x = 9
        if true
            if true
                x = 11
            else
                x = 10
            endif
        endif
        )EOF");
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & branch = std::get<MIR::BranchPtr>(irlist->block->instructions.back());

    {
        const auto & arm1 = std::get<1>(branch->branches.at(0));
        const auto & branch2 = std::get<MIR::BranchPtr>(arm1->block->instructions.back());
        const auto & sub1 = std::get<1>(branch2->branches.at(0));
        const auto & fin = std::get<MIR::JumpPtr>(sub1->block->instructions.back())->target;

        ASSERT_EQ(fin->block->instructions.size(), 2);
        const auto & it = fin->block->instructions.front();

        ASSERT_TRUE(std::holds_alternative<MIR::PhiPtr>(it));
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, it);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 4);
        const auto & phi = std::get<MIR::PhiPtr>(it);
        EXPECT_EQ(phi->left, 2);
        EXPECT_EQ(phi->right, 3);
    }

    {
        const auto & fin = std::get<1>(branch->branches.at(1));
        ASSERT_EQ(fin->block->instructions.size(), 1);
        const auto & it = fin->block->instructions.front();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, it);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 5);

        ASSERT_TRUE(std::holds_alternative<MIR::PhiPtr>(it));
        const auto & phi = std::get<MIR::PhiPtr>(it);
        EXPECT_EQ(phi->left, 1);
        EXPECT_EQ(phi->right, 4);
    }
}
