// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(fixup_phi, simple) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        else
            x = 10
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                      });

    ASSERT_EQ(irlist->block->instructions.size(), 2);

    {
        const auto & id_obj = irlist->block->instructions.front();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(id_obj));
        const auto & id = std::get<MIR::NumberPtr>(id_obj);
        EXPECT_EQ(id->value, 9);
    }

    {
        const auto & id_obj = irlist->block->instructions.back();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 3);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        EXPECT_EQ(id->version, 1);
    }
}

TEST(fixup_phi, three_branches) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        elif y()
            x = 11
        else
            x = 10
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                      });

    ASSERT_EQ(irlist->block->instructions.size(), 3);

    auto it = irlist->block->instructions.begin();

    {
        const auto & id_obj = *it;
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(id_obj));
        const auto & id = std::get<MIR::NumberPtr>(id_obj);
        EXPECT_EQ(id->value, 9);
    }

    {
        const auto & id_obj = *(++it);
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.gvn, 4);
        EXPECT_EQ(var.name, "x");

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        EXPECT_EQ(id->version, 1);
    }

    {
        const auto & id_obj = *(++it);
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 5);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        EXPECT_EQ(id->version, 4);
    }
}

TEST(fixup_phi, nested_branches) {
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

    bool progress = true;
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    while (progress) {
        progress = MIR::Passes::graph_walker(irlist, {
                                                         MIR::Passes::branch_pruning,
                                                         MIR::Passes::join_blocks,
                                                         MIR::Passes::fixup_phis,
                                                     });
    }

    ASSERT_TRUE(irlist->successors.empty());
    ASSERT_EQ(irlist->block->instructions.size(), 4);
    auto it = irlist->block->instructions.begin();

    {
        const auto & id_obj = *it;
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(id_obj));
        const auto & id = std::get<MIR::NumberPtr>(id_obj);
        EXPECT_EQ(id->value, 9);
    }

    ++it;

    {
        const auto & id_obj = *it;
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 2);

        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(id_obj));
        const auto & id = std::get<MIR::NumberPtr>(id_obj);
        EXPECT_EQ(id->value, 11);
    }

    ++it;

    {
        const auto & id_obj = *it;
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 4);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        EXPECT_EQ(id->version, 2);
    }

    ++it;

    {
        const auto & id_obj = *it;
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 5);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        EXPECT_EQ(id->version, 4);
    }
}
