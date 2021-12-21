// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

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
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, lst); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rt); },
                 });

    ASSERT_EQ(irlist.instructions.size(), 3);

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::FunctionCall>>(func_obj));
    const auto & func = std::get<std::shared_ptr<MIR::FunctionCall>>(func_obj);
    ASSERT_EQ(func->pos_args.size(), 1);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(arg_obj));
    const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(arg_obj);
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
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::insert_phis(b, data); },
                 });
    MIR::Passes::block_walker(
        &irlist, {
                     MIR::Passes::branch_pruning,
                     MIR::Passes::join_blocks,
                     MIR::Passes::fixup_phis,
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, lst); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rt); },
                 });

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::FunctionCall>>(func_obj));
    const auto & func = std::get<std::shared_ptr<MIR::FunctionCall>>(func_obj);
    ASSERT_EQ(func->pos_args.size(), 1);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(arg_obj));
    const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(arg_obj);
    ASSERT_EQ(id->value, "x");
    ASSERT_EQ(id->version, 2);
}

TEST(constant_folding, three_statements) {
    auto irlist = lower(R"EOF(
        x = 9
        y = x
        z = y
        message(z)
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::LastSeenTable rt{};
    MIR::Passes::ReplacementTable rpt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, rt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rpt); },
                 });

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::FunctionCall>>(func_obj));
    const auto & func = std::get<std::shared_ptr<MIR::FunctionCall>>(func_obj);
    ASSERT_EQ(func->pos_args.size(), 1);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(arg_obj));
    const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(arg_obj);
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
    MIR::Passes::LastSeenTable rt{};
    MIR::Passes::ReplacementTable rpt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, rt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rpt); },
                 });

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::FunctionCall>>(func_obj));
    const auto & func = std::get<std::shared_ptr<MIR::FunctionCall>>(func_obj);
    ASSERT_EQ(func->pos_args.size(), 1);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(arg_obj));
    const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(arg_obj);
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
    MIR::Passes::LastSeenTable rt{};
    MIR::Passes::ReplacementTable rpt{};

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, rt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rpt); },
                 });

    const auto & array_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::Array>>(array_obj));
    const auto & array = std::get<std::shared_ptr<MIR::Array>>(array_obj);
    ASSERT_EQ(array->value.size(), 1);

    const auto & arg_obj = array->value.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(arg_obj));
    const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(arg_obj);
    ASSERT_EQ(id->value, "x");
    ASSERT_EQ(id->version, 1);
}
