// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

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

    const auto & arm1 = std::get<1>(
        std::get<MIR::Branch>(*irlist->block->instructions.back().obj_ptr).branches.at(0));
    const auto & fin = std::get<MIR::Jump>(*arm1->block->instructions.back().obj_ptr).target;
    ASSERT_EQ(fin->block->instructions.size(), 1);

    MIR::Instruction instr = fin->block->instructions.front();
    EXPECT_EQ(instr.var.name, "x");
    EXPECT_EQ(instr.var.gvn, 3); // because value_numbering will run again

    ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*instr.obj_ptr));
    const auto & phi = std::get<MIR::Phi>(*instr.obj_ptr);
    EXPECT_EQ(phi.left, 1);
    EXPECT_EQ(phi.right, 2);
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

    const auto & arm1 = std::get<1>(
        std::get<MIR::Branch>(*irlist->block->instructions.front().obj_ptr).branches.at(0));
    const auto & fin = std::get<MIR::Jump>(*arm1->block->instructions.back().obj_ptr).target;

    ASSERT_EQ(fin->block->instructions.size(), 2);

    auto it = fin->block->instructions.begin();
    EXPECT_EQ(it->var.name, "x");
    EXPECT_EQ(it->var.gvn, 4);

    ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*it->obj_ptr));
    const auto & phi = std::get<MIR::Phi>(*it->obj_ptr);
    EXPECT_EQ(phi.left, 1);
    EXPECT_EQ(phi.right, 2);

    it++;

    EXPECT_EQ(it->var.name, "x");
    EXPECT_EQ(it->var.gvn, 5);

    ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*it->obj_ptr));
    const auto & phi2 = std::get<MIR::Phi>(*it->obj_ptr);
    EXPECT_EQ(phi2.left, 4);
    ASSERT_EQ(phi2.right, 3);
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

    const auto & branch = std::get<MIR::Branch>(*irlist->block->instructions.back().obj_ptr);

    {
        const auto & arm1 = std::get<1>(branch.branches.at(0));
        const auto & branch2 = std::get<MIR::Branch>(*arm1->block->instructions.back().obj_ptr);
        const auto & sub1 = std::get<1>(branch2.branches.at(0));
        const auto & fin = std::get<MIR::Jump>(sub1->block->instructions.back().object()).target;

        ASSERT_EQ(fin->block->instructions.size(), 2);
        const auto & it = fin->block->instructions.front();

        ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*it.obj_ptr));
        EXPECT_EQ(it.var.name, "x");
        EXPECT_EQ(it.var.gvn, 4);
        const auto & phi = std::get<MIR::Phi>(*it.obj_ptr);
        EXPECT_EQ(phi.left, 2);
        EXPECT_EQ(phi.right, 3);
    }

    {
        const auto & fin = std::get<1>(branch.branches.at(1));
        ASSERT_EQ(fin->block->instructions.size(), 1);
        const auto & it = fin->block->instructions.front();
        EXPECT_EQ(it.var.name, "x");
        EXPECT_EQ(it.var.gvn, 5);

        ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*it.obj_ptr));
        const auto & phi = std::get<MIR::Phi>(*it.obj_ptr);
        EXPECT_EQ(phi.left, 1);
        EXPECT_EQ(phi.right, 4);
    }
}
