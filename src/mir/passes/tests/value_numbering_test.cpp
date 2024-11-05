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
    MIR::Passes::GlobalValueNumbering{}(irlist);

    ASSERT_EQ(irlist->instructions.front().var.gvn, 1);
    ASSERT_EQ(irlist->instructions.back().var.gvn, 2);
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

    ASSERT_EQ(irlist->instructions.front().var.gvn, 1);
    ASSERT_EQ(irlist->instructions.back().var.gvn, 2);

    const auto & bb1 = get_con(irlist->next)->if_false;
    ASSERT_EQ(bb1->instructions.front().var.gvn, 3);

    const auto & bb2 = get_con(irlist->next)->if_true;
    ASSERT_EQ(bb2->instructions.front().var.gvn, 4);
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
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & bb1 = get_con(irlist->next)->if_true;
    EXPECT_EQ(bb1->instructions.front().var.gvn, 3);

    const auto & con2 = get_con(get_con(irlist->next)->if_false->next);

    const auto & bb2 = con2->if_false;
    EXPECT_EQ(bb2->instructions.front().var.gvn, 1);

    const auto & bb3 = con2->if_true;
    ASSERT_EQ(bb3->instructions.front().var.gvn, 2);
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

    ASSERT_EQ(irlist->instructions.size(), 2);

    {
        const auto & num_obj = irlist->instructions.front();
        ASSERT_EQ(num_obj.var.name, "x");
        ASSERT_EQ(num_obj.var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*num_obj.obj_ptr));
        const auto & num = std::get<MIR::Number>(*num_obj.obj_ptr);
        ASSERT_EQ(num.value, 9);
    }

    {
        const auto & id_obj = irlist->instructions.back();
        ASSERT_EQ(id_obj.var.name, "y");
        ASSERT_EQ(id_obj.var.gvn, 1);

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

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                      });
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                      });

    ASSERT_EQ(irlist->instructions.size(), 3);

    {
        const auto & num_obj = irlist->instructions.front();
        ASSERT_EQ(num_obj.var.name, "x");
        ASSERT_EQ(num_obj.var.gvn, 2);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*num_obj.obj_ptr));
        const auto & num = std::get<MIR::Number>(*num_obj.obj_ptr);
        ASSERT_EQ(num.value, 9);
    }

    {
        const auto & id_obj = irlist->instructions.back();
        ASSERT_EQ(id_obj.var.name, "y");
        ASSERT_EQ(id_obj.var.gvn, 1);

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

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & fin = get_bb(get_con(irlist->next)->if_false->next);
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

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    const auto & fin = get_bb(get_con(irlist->next)->if_false->next);
    ASSERT_EQ(fin->instructions.size(), 2);

    {
        const auto & phi_obj = fin->instructions.front();
        ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*phi_obj.obj_ptr));
    }

    {
        const auto & id_obj = fin->instructions.back();
        ASSERT_EQ(id_obj.var.name, "y");
        ASSERT_EQ(id_obj.var.gvn, 1);

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

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    ASSERT_EQ(irlist->instructions.size(), 3);

    {
        const auto & id_obj = irlist->instructions.back();
        ASSERT_EQ(id_obj.var.name, "z");
        ASSERT_EQ(id_obj.var.gvn, 1);

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

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    ASSERT_EQ(irlist->instructions.size(), 3);

    {
        const auto & id_obj = irlist->instructions.back();
        ASSERT_EQ(id_obj.var.name, "y");
        ASSERT_EQ(id_obj.var.gvn, 1);

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

    // Do this in two passes as otherwise the phi won't get inserted, and thus y will point at the
    // wrong thing
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});

    ASSERT_EQ(irlist->instructions.size(), 3);

    {
        const auto & num_obj = irlist->instructions.front();
        ASSERT_EQ(num_obj.var.name, "x");
        ASSERT_EQ(num_obj.var.gvn, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*num_obj.obj_ptr));
        const auto & num = std::get<MIR::Number>(*num_obj.obj_ptr);
        ASSERT_EQ(num.value, 10);
    }

    {
        const auto & arr_obj = irlist->instructions.back();
        ASSERT_TRUE(std::holds_alternative<MIR::Array>(*arr_obj.obj_ptr));
        const auto & arr = std::get<MIR::Array>(*arr_obj.obj_ptr);

        ASSERT_EQ(arr.value.size(), 1);
        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*arr.value[0].obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*arr.value[0].obj_ptr);

        ASSERT_EQ(id.value, "y");
        ASSERT_EQ(id.version, 1);
    }
}
