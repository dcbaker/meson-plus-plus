// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

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
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::value_numbering(&irlist, data);

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.front());
    ASSERT_EQ(first->var.version, 1);

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.back());
    ASSERT_EQ(last->var.version, 2);
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
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::block_walker(
        &irlist, {[&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); }});

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.front());
    ASSERT_EQ(first->var.version, 1);

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.back());
    ASSERT_EQ(last->var.version, 2);

    const auto & bb1 = get_con(irlist.next)->if_false;
    const auto & bb1_val = std::get<std::unique_ptr<MIR::Number>>(bb1->instructions.front());
    ASSERT_EQ(bb1_val->var.version, 3);

    const auto & bb2 = get_con(irlist.next)->if_true;
    const auto & bb2_val = std::get<std::unique_ptr<MIR::Number>>(bb2->instructions.front());
    ASSERT_EQ(bb2_val->var.version, 4);
}

TEST(value_numbering, three_branch) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        elif y
            x = 10
        else
            x = 11
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::block_walker(
        &irlist, {[&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); }});

    const auto & bb1 = get_con(irlist.next)->if_true;
    const auto & bb1_val = std::get<std::unique_ptr<MIR::Number>>(bb1->instructions.front());
    ASSERT_EQ(bb1_val->var.version, 1);

    const auto & con2 = get_con(get_con(irlist.next)->if_false->next);

    const auto & bb2 = con2->if_false;
    const auto & bb2_val = std::get<std::unique_ptr<MIR::Number>>(bb2->instructions.front());
    ASSERT_EQ(bb2_val->var.version, 2);

    const auto & bb3 = con2->if_true;
    const auto & bb3_val = std::get<std::unique_ptr<MIR::Number>>(bb3->instructions.front());
    ASSERT_EQ(bb3_val->var.version, 3);
}


TEST(number_uses, simple) {
    auto irlist = lower(R"EOF(
        x = 9
        y = x
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::LastSeenTable rt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, rt); },
                 });

    ASSERT_EQ(irlist.instructions.size(), 2);

    {
        const auto & num_obj = irlist.instructions.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(num_obj));
        const auto & num = std::get<std::unique_ptr<MIR::Number>>(num_obj);
        ASSERT_EQ(num->value, 9);
        ASSERT_EQ(num->var.name, "x");
        ASSERT_EQ(num->var.version, 1);
    }

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(id_obj));
        const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(id_obj);
        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 1);
        ASSERT_EQ(id->var.name, "y");
        ASSERT_EQ(id->var.version, 1);
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
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::LastSeenTable rt{};

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
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, rt); },
                 });

    ASSERT_EQ(irlist.instructions.size(), 3);

    {
        const auto & num_obj = irlist.instructions.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(num_obj));
        const auto & num = std::get<std::unique_ptr<MIR::Number>>(num_obj);
        ASSERT_EQ(num->value, 9);
        ASSERT_EQ(num->var.name, "x");
        ASSERT_EQ(num->var.version, 2);
    }

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(id_obj));
        const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(id_obj);
        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 3);
        ASSERT_EQ(id->var.name, "y");
        ASSERT_EQ(id->var.version, 1);
    }
}

TEST(number_uses, three_statements) {
    auto irlist = lower(R"EOF(
        x = 9
        y = x
        z = y
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::LastSeenTable rt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, rt); },
                 });

    ASSERT_EQ(irlist.instructions.size(), 3);

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(id_obj));
        const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(id_obj);
        ASSERT_EQ(id->value, "y");
        ASSERT_EQ(id->version, 1);
        ASSERT_EQ(id->var.name, "z");
        ASSERT_EQ(id->var.version, 1);
    }
}

TEST(number_uses, redefined_value) {
    auto irlist = lower(R"EOF(
        x = 9
        x = 10
        y = x
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::LastSeenTable rt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, rt); },
                 });

    ASSERT_EQ(irlist.instructions.size(), 3);

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(id_obj));
        const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(id_obj);
        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 2);
        ASSERT_EQ(id->var.name, "y");
        ASSERT_EQ(id->var.version, 1);
    }
}

TEST(number_uses, in_array) {
    auto irlist = lower(R"EOF(
        x = 10
        y = [x]
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::LastSeenTable rt{};

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, rt); },
                 });

    ASSERT_EQ(irlist.instructions.size(), 2);

    {
        const auto & num_obj = irlist.instructions.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(num_obj));
        const auto & num = std::get<std::unique_ptr<MIR::Number>>(num_obj);
        ASSERT_EQ(num->value, 10);
        ASSERT_EQ(num->var.name, "x");
        ASSERT_EQ(num->var.version, 1);
    }

    {
        const auto & arr_obj = irlist.instructions.back();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Array>>(arr_obj));
        const auto & arr = std::get<std::unique_ptr<MIR::Array>>(arr_obj);

        ASSERT_EQ(arr->value.size(), 1);
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(arr->value[0]));
        const auto & id = std::get<std::unique_ptr<MIR::Identifier>>(arr->value[0]);

        ASSERT_EQ(id->value, "x");
        ASSERT_EQ(id->version, 1);
    }
}
