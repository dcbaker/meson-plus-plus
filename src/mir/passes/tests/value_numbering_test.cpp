// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

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
    MIR::Passes::value_numbering(irlist, data);

    ASSERT_EQ(irlist.instructions.front().var.version, 1);
    ASSERT_EQ(irlist.instructions.back().var.version, 2);
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
        irlist, {[&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); }});

    ASSERT_EQ(irlist.instructions.front().var.version, 1);
    ASSERT_EQ(irlist.instructions.back().var.version, 2);

    const auto & bb1 = get_con(irlist.next)->if_false;
    ASSERT_EQ(bb1->instructions.front().var.version, 3);

    const auto & bb2 = get_con(irlist.next)->if_true;
    ASSERT_EQ(bb2->instructions.front().var.version, 4);
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
        irlist, {[&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); }});

    const auto & bb1 = get_con(irlist.next)->if_true;
    ASSERT_EQ(bb1->instructions.front().var.version, 1);

    const auto & con2 = get_con(get_con(irlist.next)->if_false->next);

    const auto & bb2 = con2->if_false;
    ASSERT_EQ(bb2->instructions.front().var.version, 2);

    const auto & bb3 = con2->if_true;
    ASSERT_EQ(bb3->instructions.front().var.version, 3);
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
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, rt); },
                });

    ASSERT_EQ(irlist.instructions.size(), 2);

    {
        const auto & num_obj = irlist.instructions.front();
        ASSERT_EQ(num_obj.var.name, "x");
        ASSERT_EQ(num_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*num_obj.obj_ptr));
        const auto & num = std::get<MIR::Number>(*num_obj.obj_ptr);
        ASSERT_EQ(num.value, 9);
    }

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_EQ(id_obj.var.name, "y");
        ASSERT_EQ(id_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, "x");
        ASSERT_EQ(id.version, 1);
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
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                });
    MIR::Passes::block_walker(
        irlist, {
                    MIR::Passes::branch_pruning,
                    MIR::Passes::join_blocks,
                    MIR::Passes::fixup_phis,
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, rt); },
                });

    ASSERT_EQ(irlist.instructions.size(), 3);

    {
        const auto & num_obj = irlist.instructions.front();
        ASSERT_EQ(num_obj.var.name, "x");
        ASSERT_EQ(num_obj.var.version, 2);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*num_obj.obj_ptr));
        const auto & num = std::get<MIR::Number>(*num_obj.obj_ptr);
        ASSERT_EQ(num.value, 9);
    }

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_EQ(id_obj.var.name, "y");
        ASSERT_EQ(id_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, "x");
        ASSERT_EQ(id.version, 3);
    }
}

TEST(number_uses, with_phi_no_pruning_in_func_call) {
    auto irlist = lower(R"EOF(
        if some_var
            x = 9
        else
            x = 10
        endif
        message(x)
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::LastSeenTable rt{};

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, rt); },
                });

    const auto & fin = get_bb(get_con(irlist.next)->if_false->next);
    ASSERT_EQ(fin->instructions.size(), 2);

    {
        const auto & phi_obj = fin->instructions.front();
        ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*phi_obj.obj_ptr));
    }

    {
        const auto & func_obj = fin->instructions.back();
        ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*func_obj.obj_ptr));
        const auto & func = std::get<MIR::FunctionCall>(*func_obj.obj_ptr);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*func.pos_args.front().obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*func.pos_args.front().obj_ptr);
        ASSERT_EQ(id.value, "x");
        ASSERT_EQ(id.version, 3);
    }
}

TEST(number_uses, with_phi_no_pruning) {
    auto irlist = lower(R"EOF(
        if some_var
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
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, rt); },
                });

    const auto & fin = get_bb(get_con(irlist.next)->if_false->next);
    ASSERT_EQ(fin->instructions.size(), 2);

    {
        const auto & phi_obj = fin->instructions.front();
        ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*phi_obj.obj_ptr));
    }

    {
        const auto & id_obj = fin->instructions.back();
        ASSERT_EQ(id_obj.var.name, "y");
        ASSERT_EQ(id_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, "x");
        ASSERT_EQ(id.version, 3);
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
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, rt); },
                });

    ASSERT_EQ(irlist.instructions.size(), 3);

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_EQ(id_obj.var.name, "z");
        ASSERT_EQ(id_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, "y");
        ASSERT_EQ(id.version, 1);
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
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, rt); },
                });

    ASSERT_EQ(irlist.instructions.size(), 3);

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_EQ(id_obj.var.name, "y");
        ASSERT_EQ(id_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, "x");
        ASSERT_EQ(id.version, 2);
    }
}

TEST(number_uses, in_array) {
    auto irlist = lower(R"EOF(
        x = 10
        y = x
        y = [y]
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::LastSeenTable rt{};

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, rt); },
                });

    ASSERT_EQ(irlist.instructions.size(), 3);

    {
        const auto & num_obj = irlist.instructions.front();
        ASSERT_EQ(num_obj.var.name, "x");
        ASSERT_EQ(num_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*num_obj.obj_ptr));
        const auto & num = std::get<MIR::Number>(*num_obj.obj_ptr);
        ASSERT_EQ(num.value, 10);
    }

    {
        const auto & arr_obj = irlist.instructions.back();
        ASSERT_TRUE(std::holds_alternative<MIR::Array>(*arr_obj.obj_ptr));
        const auto & arr = std::get<MIR::Array>(*arr_obj.obj_ptr);

        ASSERT_EQ(arr.value.size(), 1);
        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*arr.value[0].obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*arr.value[0].obj_ptr);

        ASSERT_EQ(id.value, "y");
        ASSERT_EQ(id.version, 1);
    }
}
