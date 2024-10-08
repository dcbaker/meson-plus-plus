// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

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
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    MIR::Passes::UsageNumbering{},
                    MIR::Passes::ConstantFolding{},
                });

    ASSERT_EQ(irlist.instructions.size(), 3);

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*func_obj.obj_ptr));
    const auto & func = std::get<MIR::FunctionCall>(*func_obj.obj_ptr);
    ASSERT_EQ(func.pos_args.size(), 1);

    const auto & arg_obj = func.pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*arg_obj.obj_ptr));
    const auto & id = std::get<MIR::Identifier>(*arg_obj.obj_ptr);
    ASSERT_EQ(id.value, "x");
    ASSERT_EQ(id.version, 1);
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
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                });
    MIR::Passes::block_walker(irlist, {
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                          MIR::Passes::UsageNumbering{},
                                          MIR::Passes::ConstantFolding{},
                                      });

    auto it = irlist.instructions.begin();

    ASSERT_EQ(it->var.version, 2);
    ASSERT_EQ(it->var.name, "x");

    const auto & num_obj = *(it);
    ASSERT_TRUE(std::holds_alternative<MIR::Number>(*num_obj.obj_ptr));
    const auto & num = std::get<MIR::Number>(*num_obj.obj_ptr);
    ASSERT_EQ(num.value, 9);

    // This was the Phi
    const auto & phi_obj = *(++it);
    ASSERT_EQ(it->var.name, "x");
    ASSERT_EQ(it->var.version, 3);

    ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*phi_obj.obj_ptr));
    const auto & phi = std::get<MIR::Identifier>(*phi_obj.obj_ptr);
    ASSERT_EQ(phi.value, "x");
    ASSERT_EQ(phi.version, 2);

    {
        const auto & id_obj = *(++it);
        ASSERT_EQ(it->var.name, "y");
        ASSERT_EQ(it->var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, "x");
        ASSERT_EQ(id.version, 2);
    }

    {
        const auto & func_obj = *(++it);
        ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*func_obj.obj_ptr));
        const auto & func = std::get<MIR::FunctionCall>(*func_obj.obj_ptr);
        ASSERT_EQ(func.pos_args.size(), 1);

        const auto & arg_obj = func.pos_args.front();
        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*arg_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*arg_obj.obj_ptr);
        ASSERT_EQ(id.value, "x");
        ASSERT_EQ(id.version, 2);
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
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    MIR::Passes::UsageNumbering{},
                    MIR::Passes::ConstantFolding{},
                });

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*func_obj.obj_ptr));
    const auto & func = std::get<MIR::FunctionCall>(*func_obj.obj_ptr);
    ASSERT_EQ(func.pos_args.size(), 1);

    const auto & arg_obj = func.pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*arg_obj.obj_ptr));
    const auto & id = std::get<MIR::Identifier>(*arg_obj.obj_ptr);
    ASSERT_EQ(id.value, "x");
    ASSERT_EQ(id.version, 1);
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
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    MIR::Passes::UsageNumbering{},
                    MIR::Passes::ConstantFolding{},
                });

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*func_obj.obj_ptr));
    const auto & func = std::get<MIR::FunctionCall>(*func_obj.obj_ptr);
    ASSERT_EQ(func.pos_args.size(), 1);

    const auto & arg_obj = func.pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*arg_obj.obj_ptr));
    const auto & id = std::get<MIR::Identifier>(*arg_obj.obj_ptr);
    ASSERT_EQ(id.value, "x");
    ASSERT_EQ(id.version, 2);
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
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    MIR::Passes::UsageNumbering{},
                    MIR::Passes::ConstantFolding{},
                });

    auto it = irlist.instructions.begin();
    {
        const auto & id_obj = *it;
        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*id_obj.obj_ptr));
        ASSERT_EQ(it->var.name, "x");
        ASSERT_EQ(it->var.version, 1);
    }

    {
        const auto & id_obj = *(++it);
        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, "x");
        ASSERT_EQ(id.version, 1);
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
