// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(constant_propogation, phi_should_not_propogate) {
    auto irlist = lower(R"EOF(
        if some_var
            x = 9
        else
            x = 10
        endif
        message(x)
        )EOF");
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ValueTable vt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, vt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::insert_phis(b, vt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, lst); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_propogation(b, pt); },
                 });

    const auto & fin = get_bb(get_con(irlist.next)->if_true->next);

    ASSERT_EQ(fin->instructions.size(), 2);

    const auto & phi_obj = fin->instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Phi>>(phi_obj));
    const auto & phi = std::get<std::unique_ptr<MIR::Phi>>(phi_obj);
    ASSERT_EQ(phi->var.name, "x");
    ASSERT_EQ(phi->var.version, 3);

    const auto & func_obj = fin->instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::FunctionCall>>(func_obj));
    const auto & func = std::get<std::shared_ptr<MIR::FunctionCall>>(func_obj);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(arg_obj));
    const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(arg_obj);
    ASSERT_EQ(id->value, "x");
    ASSERT_EQ(id->version, 3);
}

TEST(constant_propogation, function_arguments) {
    auto irlist = lower(R"EOF(
        if true
            x = 'true'
        else
            x = 'false'
        endif
        message(x)
        )EOF");
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ValueTable vt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, vt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::insert_phis(b, vt); },
                     MIR::Passes::branch_pruning,
                     MIR::Passes::join_blocks,
                     MIR::Passes::fixup_phis,
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, lst); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_propogation(b, pt); },
                 });

    ASSERT_EQ(irlist.instructions.size(), 2);

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::FunctionCall>>(func_obj));
    const auto & func = std::get<std::shared_ptr<MIR::FunctionCall>>(func_obj);

    const auto & arg_obj = func->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::String>>(arg_obj));
    const auto & str = std::get<std::shared_ptr<MIR::String>>(arg_obj);
    ASSERT_EQ(str->value, "true");
}

TEST(constant_propogation, array) {
    auto irlist = lower(R"EOF(
        if true
            x = 'true'
        else
            x = 'false'
        endif
        y = [x]
        )EOF");
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ValueTable vt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, vt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::insert_phis(b, vt); },
                     MIR::Passes::branch_pruning,
                     MIR::Passes::join_blocks,
                     MIR::Passes::fixup_phis,
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, lst); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rt); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::constant_propogation(b, pt); },
                 });

    ASSERT_EQ(irlist.instructions.size(), 2);

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::Array>>(func_obj));
    const auto & func = std::get<std::shared_ptr<MIR::Array>>(func_obj);

    const auto & arg_obj = func->value.front();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::String>>(arg_obj));
    const auto & str = std::get<std::shared_ptr<MIR::String>>(arg_obj);
    ASSERT_EQ(str->value, "true");
}
