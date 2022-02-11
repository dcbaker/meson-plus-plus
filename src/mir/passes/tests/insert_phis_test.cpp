// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

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
    std::unordered_map<std::string, uint32_t> data{};

    MIR::Passes::block_walker(
        irlist, {
                     [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                 });

    const auto & fin = get_bb(get_con(irlist.next)->if_false->next);
    ASSERT_EQ(fin->instructions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Phi>>(fin->instructions.front()));

    const auto & phi = std::get<std::unique_ptr<MIR::Phi>>(fin->instructions.front());
    ASSERT_EQ(phi->left, 2);
    ASSERT_EQ(phi->right, 1);
    ASSERT_EQ(phi->var.name, "x");
    ASSERT_EQ(phi->var.version, 3); // because value_numbering will run again
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

    MIR::Passes::block_walker(
        irlist, {
                     [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                 });

    const auto & fin = get_bb(get_con(irlist.next)->if_true->next);
    ASSERT_EQ(fin->instructions.size(), 2);

    auto it = fin->instructions.begin();

    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Phi>>(*it));
    const auto & phi = std::get<std::unique_ptr<MIR::Phi>>(*it);
    ASSERT_EQ(phi->left, 1);
    ASSERT_EQ(phi->right, 3);
    ASSERT_EQ(phi->var.name, "x");
    ASSERT_EQ(phi->var.version, 4);

    it++;

    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Phi>>(*it));
    const auto & phi2 = std::get<std::unique_ptr<MIR::Phi>>(*it);
    ASSERT_EQ(phi2->left, 4);
    ASSERT_EQ(phi2->right, 2);
    ASSERT_EQ(phi2->var.name, "x");
    ASSERT_EQ(phi2->var.version, 5);
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
    std::unordered_map<std::string, uint32_t> data{};

    MIR::Passes::block_walker(
        irlist, {
                     [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                 });

    {
        const auto & fin =
            get_bb(get_bb(get_con(get_bb(get_con(irlist.next)->if_true)->next)->if_true)->next);
        ASSERT_EQ(fin->instructions.size(), 1);
        const auto & it = fin->instructions.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Phi>>(it));
        const auto & phi = std::get<std::unique_ptr<MIR::Phi>>(it);
        ASSERT_EQ(phi->left, 3);
        ASSERT_EQ(phi->right, 2);
        ASSERT_EQ(phi->var.name, "x");
        ASSERT_EQ(phi->var.version, 4);
    }

    {
        const auto & fin = get_bb(get_con(irlist.next)->if_false);
        ASSERT_EQ(fin->instructions.size(), 1);
        const auto & it = fin->instructions.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Phi>>(it));
        const auto & phi = std::get<std::unique_ptr<MIR::Phi>>(it);
        ASSERT_EQ(phi->left, 1);
        ASSERT_EQ(phi->right, 4);
        ASSERT_EQ(phi->var.name, "x");
        ASSERT_EQ(phi->var.version, 5);
    }
}
