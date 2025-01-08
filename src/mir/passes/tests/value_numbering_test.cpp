// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"

#include "test_utils.hpp"

TEST(value_numbering, simple) {
    auto irlist = lower(R"EOF(
        x = 7
        x = 8
        )EOF");
    MIR::Passes::GlobalValueNumbering{}(irlist);

    ASSERT_EQ(std::visit(MIR::VariableGetter{}, irlist->block->instructions.front()).gvn, 1);
    ASSERT_EQ(std::visit(MIR::VariableGetter{}, irlist->block->instructions.back()).gvn, 2);
}

TEST(value_numbering, branching) {
    auto irlist = lower(R"EOF(
        x = 7
        x = 8
        if true
            x = 9
        else
            x = 10
        endif
        )EOF");
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    auto it = irlist->block->instructions.begin();

    EXPECT_EQ(std::visit(MIR::VariableGetter{}, *(it++)).gvn, 1);
    EXPECT_EQ(std::visit(MIR::VariableGetter{}, *(it++)).gvn, 2);

    const auto & branch = std::get<MIR::BranchPtr>(irlist->block->instructions.back());

    const auto & bb1 = std::get<1>(branch->branches.at(0));
    EXPECT_EQ(std::visit(MIR::VariableGetter{}, bb1->block->instructions.front()).gvn, 3);

    const auto & bb2 = std::get<1>(branch->branches.at(1));
    EXPECT_EQ(std::visit(MIR::VariableGetter{}, bb2->block->instructions.front()).gvn, 4);
}

TEST(value_numbering, three_branch) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        elif y()
            x = 10
        else
            x = 11
        endif
        )EOF");
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & branches = std::get<MIR::BranchPtr>(irlist->block->instructions.back());

    const auto & bb1 = std::get<1>(branches->branches.at(0));
    EXPECT_EQ(std::visit(MIR::VariableGetter{}, bb1->block->instructions.front()).gvn, 1);

    const auto & bb2 = std::get<1>(branches->branches.at(1));
    EXPECT_EQ(std::visit(MIR::VariableGetter{}, bb2->block->instructions.front()).gvn, 2);

    const auto & bb3 = std::get<1>(branches->branches.at(2));
    EXPECT_EQ(std::visit(MIR::VariableGetter{}, bb3->block->instructions.front()).gvn, 3);
}

TEST(number_uses, simple) {
    auto irlist = lower(R"EOF(
        x = 9
        y = x
        )EOF");

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                      });

    ASSERT_EQ(irlist->block->instructions.size(), 2);

    {
        const auto & num_obj = irlist->block->instructions.front();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, num_obj);
        ASSERT_EQ(var.name, "x");
        ASSERT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(num_obj));
        const auto & num = std::get<MIR::NumberPtr>(num_obj);
        ASSERT_EQ(num->value, 9);
    }

    {
        const auto & id_obj = irlist->block->instructions.back();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        ASSERT_EQ(var.name, "y");
        ASSERT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 1);
    }
}

TEST(number_uses, with_phi) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        else
            x = 10
        endif
        y = x
        )EOF");

    MIR::Passes::Printer printer{};

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          std::ref(printer),
                                      });
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::branch_pruning,
                                          std::ref(printer),
                                          MIR::Passes::join_blocks,
                                          std::ref(printer),
                                          MIR::Passes::fixup_phis,
                                          std::ref(printer),
                                      });

    ASSERT_EQ(irlist->block->instructions.size(), 3);

    {
        const auto & num_obj = irlist->block->instructions.front();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, num_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(num_obj));
        const auto & num = std::get<MIR::NumberPtr>(num_obj);
        EXPECT_EQ(num->value, 9);
    }

    {
        const auto & id_obj = irlist->block->instructions.back();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "y");
        EXPECT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        EXPECT_EQ(id->value, "x");
        EXPECT_EQ(id->version, 3);
    }
}

TEST(number_uses, with_phi_no_pruning_in_func_call) {
    auto irlist = lower(R"EOF(
        if some_var()
            x = 9
        else
            x = 10
        endif
        message(x)
        )EOF");

    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & branches = std::get<MIR::BranchPtr>(irlist->block->instructions.front());
    const auto & arm = std::get<1>(branches->branches.at(0));
    const auto & tail = std::get<MIR::JumpPtr>(arm->block->instructions.back())->target;

    ASSERT_EQ(tail->block->instructions.size(), 2);

    {
        const auto & phi_obj = tail->block->instructions.front();
        ASSERT_TRUE(std::holds_alternative<MIR::PhiPtr>(phi_obj));
    }

    {
        const auto & func_obj = tail->block->instructions.back();
        ASSERT_TRUE(std::holds_alternative<MIR::FunctionCallPtr>(func_obj));
        const auto & func = std::get<MIR::FunctionCallPtr>(func_obj);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(func->pos_args.front()));
        const auto & id = std::get<MIR::IdentifierPtr>(func->pos_args.front());
        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 3);
    }
}

TEST(number_uses, with_phi_no_pruning) {
    auto irlist = lower(R"EOF(
        if some_var()
            x = 9
        else
            x = 10
        endif
        y = x
        )EOF");

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & branches = std::get<MIR::BranchPtr>(irlist->block->instructions.front());
    const auto & arm = std::get<1>(branches->branches.at(0));
    const auto & tail = std::get<MIR::JumpPtr>(arm->block->instructions.back())->target;

    {
        const auto & phi_obj = tail->block->instructions.front();
        ASSERT_TRUE(std::holds_alternative<MIR::PhiPtr>(phi_obj));
    }

    {
        const auto & id_obj = tail->block->instructions.back();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        ASSERT_EQ(var.name, "y");
        ASSERT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 3);
    }
}

TEST(number_uses, three_statements) {
    auto irlist = lower(R"EOF(
        x = 9
        y = x
        z = y
        )EOF");

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    ASSERT_EQ(irlist->block->instructions.size(), 3);

    {
        const auto & id_obj = irlist->block->instructions.back();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        ASSERT_EQ(var.name, "z");
        ASSERT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        ASSERT_EQ(id->value, "y");
        ASSERT_EQ(id->version, 1);
    }
}

TEST(number_uses, redefined_value) {
    auto irlist = lower(R"EOF(
        x = 9
        x = 10
        y = x
        )EOF");

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    ASSERT_EQ(irlist->block->instructions.size(), 3);

    {
        const auto & id_obj = irlist->block->instructions.back();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        ASSERT_EQ(var.name, "y");
        ASSERT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 2);
    }
}

TEST(number_uses, in_array) {
    auto irlist = lower(R"EOF(
        x = 10
        y = x
        y = [y]
        )EOF");

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    ASSERT_EQ(irlist->block->instructions.size(), 3);

    {
        const auto & num_obj = irlist->block->instructions.front();
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, num_obj);
        ASSERT_EQ(var.name, "x");
        ASSERT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(num_obj));
        const auto & num = std::get<MIR::NumberPtr>(num_obj);
        ASSERT_EQ(num->value, 10);
    }

    {
        const auto & arr_obj = irlist->block->instructions.back();
        ASSERT_TRUE(std::holds_alternative<MIR::ArrayPtr>(arr_obj));
        const auto & arr = std::get<MIR::ArrayPtr>(arr_obj);

        ASSERT_EQ(arr->value.size(), 1);
        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(arr->value[0]));
        const auto & id = std::get<MIR::IdentifierPtr>(arr->value[0]);

        ASSERT_EQ(id->value, "y");
        ASSERT_EQ(id->version, 1);
    }
}
