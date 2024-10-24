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

    MIR::Passes::block_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                      });

    const auto & fin = get_bb(get_con(irlist.next)->if_false->next);
    ASSERT_EQ(fin->instructions.size(), 1);

    MIR::Instruction instr = fin->instructions.front();
    ASSERT_EQ(instr.var.name, "x");
    ASSERT_EQ(instr.var.gvn, 3); // because value_numbering will run again

    ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*instr.obj_ptr));
    const auto & phi = std::get<MIR::Phi>(*instr.obj_ptr);
    ASSERT_EQ(phi.left, 2);
    ASSERT_EQ(phi.right, 1);
}

TEST(insert_phi, three_branches) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        elif y
            x = 11
        else
            x = 10
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    MIR::Passes::block_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & fin = get_bb(get_con(irlist.next)->if_true->next);
    ASSERT_EQ(fin->instructions.size(), 2);

    auto it = fin->instructions.begin();
    EXPECT_EQ(it->var.name, "x");
    EXPECT_EQ(it->var.gvn, 4);

    ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*it->obj_ptr));
    const auto & phi = std::get<MIR::Phi>(*it->obj_ptr);
    EXPECT_EQ(phi.left, 3);
    EXPECT_EQ(phi.right, 2);

    it++;

    EXPECT_EQ(it->var.name, "x");
    EXPECT_EQ(it->var.gvn, 5);

    ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*it->obj_ptr));
    const auto & phi2 = std::get<MIR::Phi>(*it->obj_ptr);
    EXPECT_EQ(phi2.left, 4);
    ASSERT_EQ(phi2.right, 1);
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
    MIR::Passes::block_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    {
        const auto & fin =
            get_bb(get_bb(get_con(get_bb(get_con(irlist.next)->if_true)->next)->if_true)->next);
        ASSERT_EQ(fin->instructions.size(), 1);
        const auto & it = fin->instructions.front();
        ASSERT_EQ(it.var.name, "x");
        ASSERT_EQ(it.var.gvn, 4);

        ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*it.obj_ptr));
        const auto & phi = std::get<MIR::Phi>(*it.obj_ptr);
        ASSERT_EQ(phi.left, 3);
        ASSERT_EQ(phi.right, 2);
    }

    {
        const auto & fin = get_bb(get_con(irlist.next)->if_false);
        ASSERT_EQ(fin->instructions.size(), 1);
        const auto & it = fin->instructions.front();
        ASSERT_EQ(it.var.name, "x");
        ASSERT_EQ(it.var.gvn, 5);

        ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*it.obj_ptr));
        const auto & phi = std::get<MIR::Phi>(*it.obj_ptr);
        ASSERT_EQ(phi.left, 1);
        ASSERT_EQ(phi.right, 4);
    }
}
