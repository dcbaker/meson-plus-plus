// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(constant_folding, simple) {
    auto irlist = lower(R"EOF(
        x = 9
        y = x
        message(y)
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          MIR::Passes::ConstantFolding{},
                                      });

    ASSERT_EQ(irlist->block->instructions.size(), 3);

    const auto & func_obj = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCallPtr>(func_obj));
    const auto & func = std::get<MIR::FunctionCallPtr>(func_obj);
    ASSERT_EQ(func->pos_args.size(), 1);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(arg_obj));
    const auto & id = std::get<MIR::IdentifierPtr>(arg_obj);
    ASSERT_EQ(id->value, "x");
    ASSERT_EQ(id->version, 1);
}

TEST(constant_folding, with_phi) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        else
            x = 10
        endif
        y = x
        message(y)
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                      });
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                          MIR::Passes::ConstantFolding{},
                                      });

    auto it = irlist->block->instructions.begin();

    {
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, *it);
        EXPECT_EQ(var.gvn, 1);
        EXPECT_EQ(var.name, "x");

        const auto & num_obj = *(it);
        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(num_obj));
        const auto & num = std::get<MIR::NumberPtr>(num_obj);
        EXPECT_EQ(num->value, 9);
    }

    // This was the Phi
    const auto & phi_obj = *(++it);
    {
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, phi_obj);
        EXPECT_EQ(var.name, "x");
        EXPECT_EQ(var.gvn, 3);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(phi_obj));
        const auto & phi = std::get<MIR::IdentifierPtr>(phi_obj);
        EXPECT_EQ(phi->value, "x");
        EXPECT_EQ(phi->version, 1);
    }

    {
        const auto & id_obj = *(++it);
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, id_obj);
        EXPECT_EQ(var.name, "y");
        EXPECT_EQ(var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        EXPECT_EQ(id->value, "x");
        EXPECT_EQ(id->version, 1);
    }

    {
        const auto & func_obj = *(++it);
        ASSERT_TRUE(std::holds_alternative<MIR::FunctionCallPtr>(func_obj));
        const auto & func = std::get<MIR::FunctionCallPtr>(func_obj);
        EXPECT_EQ(func->pos_args.size(), 1);

        const auto & arg_obj = func->pos_args.front();
        EXPECT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(arg_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(arg_obj);
        EXPECT_EQ(id->value, "x");
        EXPECT_EQ(id->version, 1);
    }
}

TEST(constant_folding, three_statements) {
    auto irlist = lower(R"EOF(
        x = 9
        y = x
        z = y
        message(z)
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          MIR::Passes::ConstantFolding{},
                                      });

    const auto & func_obj = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCallPtr>(func_obj));
    const auto & func = std::get<MIR::FunctionCallPtr>(func_obj);
    ASSERT_EQ(func->pos_args.size(), 1);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(arg_obj));
    const auto & id = std::get<MIR::IdentifierPtr>(arg_obj);
    ASSERT_EQ(id->value, "x");
    ASSERT_EQ(id->version, 1);
}

TEST(constant_folding, redefined_value) {
    auto irlist = lower(R"EOF(
        x = 9
        x = 10
        y = x
        message(y)
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          MIR::Passes::ConstantFolding{},
                                      });

    const auto & func_obj = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCallPtr>(func_obj));
    const auto & func = std::get<MIR::FunctionCallPtr>(func_obj);
    ASSERT_EQ(func->pos_args.size(), 1);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(arg_obj));
    const auto & id = std::get<MIR::IdentifierPtr>(arg_obj);
    ASSERT_EQ(id->value, "x");
    ASSERT_EQ(id->version, 2);
}

TEST(constant_folding, in_array) {
    auto irlist = lower(R"EOF(
        x = 10
        y = x
        y = [y]
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          MIR::Passes::ConstantFolding{},
                                      });

    auto it = irlist->block->instructions.begin();
    {
        const auto & id_obj = *it;
        ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(id_obj));
        const MIR::Variable & var = std::visit(MIR::VariableGetter{}, *it);
        ASSERT_EQ(var.name, "x");
        ASSERT_EQ(var.gvn, 1);
    }

    {
        const auto & id_obj = *(++it);
        ASSERT_TRUE(std::holds_alternative<MIR::IdentifierPtr>(id_obj));
        const auto & id = std::get<MIR::IdentifierPtr>(id_obj);
        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 1);
    }
}

TEST(variable, less_than) {
    {
        const MIR::Variable v1{"name", 1};
        const MIR::Variable v2{"name", 2};
        ASSERT_LT(v1, v2);
    }
    {
        const MIR::Variable v1{"name", 1};
        const MIR::Variable v2{"name", 2};
        ASSERT_FALSE(v2 < v1);
    }
    {
        const MIR::Variable v1{"a", 1};
        const MIR::Variable v2{"b", 1};
        ASSERT_LT(v1, v2);
    }
}
