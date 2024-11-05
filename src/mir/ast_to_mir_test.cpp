// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>
#include <sstream>
#include <variant>

#include "ast_to_mir.hpp"
#include "driver.hpp"
#include "mir.hpp"

namespace {

std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in) {
    Frontend::Driver drv{};
    std::istringstream stream{in};
    drv.name = "/home/foo/bar/meson.build";
    auto block = drv.parse(stream);
    return block;
}

std::shared_ptr<MIR::CFGNode> lower(const std::string & in) {
    auto block = parse(in);
    const MIR::State::Persistant pstate{"foo/src", "foo/build", ""};
    auto ir = MIR::lower_ast(block, pstate);
    return ir.root;
}

} // namespace

TEST(ast_to_ir, number) {
    auto irlist = lower("7");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Number>(*obj.obj_ptr));
    const auto & ir = std::get<MIR::Number>(*obj.obj_ptr);
    ASSERT_EQ(ir.value, 7);
}

TEST(ast_to_ir, boolean) {
    auto irlist = lower("true");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Boolean>(*obj.obj_ptr));
    const auto & ir = std::get<MIR::Boolean>(*obj.obj_ptr);
    ASSERT_EQ(ir.value, true);
}

TEST(ast_to_ir, string) {
    auto irlist = lower("'true'");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::String>(*obj.obj_ptr));
    const auto & ir = std::get<MIR::String>(*obj.obj_ptr);
    ASSERT_EQ(ir.value, "true");
}

TEST(ast_to_ir, array) {
    auto irlist = lower("['a', 'b', 1, [2]]");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Array>(*obj.obj_ptr));

    const auto & arr = std::get<MIR::Array>(*obj.obj_ptr);

    const auto & arr0 = std::get<MIR::String>(*arr.value[0].obj_ptr);
    ASSERT_EQ(arr0.value, "a");

    const auto & arr1 = std::get<MIR::String>(*arr.value[1].obj_ptr);
    ASSERT_EQ(arr1.value, "b");

    const auto & arr2 = std::get<MIR::Number>(*arr.value[2].obj_ptr);
    ASSERT_EQ(arr2.value, 1);

    const auto & arr3 = std::get<MIR::Array>(*arr.value[3].obj_ptr);
    const auto & arr3_1 = std::get<MIR::Number>(*arr3.value[0].obj_ptr);
    ASSERT_EQ(arr3_1.value, 2);
}

TEST(ast_to_ir, dict) {
    auto irlist = lower("{'str': 1}");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Dict>(*obj.obj_ptr));

    const auto & dict = std::get<MIR::Dict>(*obj.obj_ptr);

    const auto & val = std::get<MIR::Number>(*dict.value.at("str").obj_ptr);
    ASSERT_EQ(val.value, 1);
}

TEST(ast_to_ir, simple_function) {
    auto irlist = lower("has_no_args()");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));

    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_EQ(ir.name, "has_no_args");
    const auto & arguments = ir.pos_args;
    ASSERT_TRUE(arguments.empty());
}

TEST(ast_to_ir, simple_method) {
    auto irlist = lower("obj.method()");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));

    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_EQ(ir.name, "method");
    ASSERT_EQ(ir.source_dir, ""); // We want to ensure this isn't meson.build
    ASSERT_TRUE(!std::holds_alternative<std::monostate>(*ir.holder.obj_ptr));
    // ASSERT_EQ(ir.holder.value(), MIR::Identifier{"obj"});
    ASSERT_TRUE(ir.pos_args.empty());
    ASSERT_TRUE(ir.kw_args.empty());
}

TEST(ast_to_ir, chained_method) {
    auto irlist = lower("obj.method().chained()");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));

    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_EQ(ir.name, "chained");
    ASSERT_TRUE(!std::holds_alternative<std::monostate>(*ir.holder.obj_ptr));
    ASSERT_TRUE(ir.pos_args.empty());
    ASSERT_TRUE(ir.kw_args.empty());

    const auto & ir2 = std::get<MIR::FunctionCall>(*ir.holder.obj_ptr);
    ASSERT_EQ(ir2.name, "method");
    ASSERT_TRUE(!std::holds_alternative<std::monostate>(*ir2.holder.obj_ptr));
    ASSERT_TRUE(ir2.pos_args.empty());
    ASSERT_TRUE(ir2.kw_args.empty());

    ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*ir2.holder.obj_ptr));
}

TEST(ast_to_ir, method_in_function) {
    auto irlist = lower("obj.method() == 0");
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));
    const auto & eq = std::get<MIR::FunctionCall>(obj.object());
    ASSERT_EQ(eq.name, "rel_eq");
    ASSERT_EQ(eq.pos_args.size(), 2);
    ASSERT_TRUE(std::holds_alternative<std::monostate>(eq.holder.object()));
}

TEST(ast_to_ir, function_positional_arguments_only) {
    auto irlist = lower("has_args(1, 2, 3)");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));

    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_EQ(ir.name, "has_args");

    const auto & arguments = ir.pos_args;
    ASSERT_EQ(arguments.size(), 3);
    ASSERT_EQ(std::get<MIR::Number>(*arguments[0].obj_ptr).value, 1);
    ASSERT_EQ(std::get<MIR::Number>(*arguments[1].obj_ptr).value, 2);
    ASSERT_EQ(std::get<MIR::Number>(*arguments[2].obj_ptr).value, 3);
}

TEST(ast_to_ir, function_keyword_arguments_only) {
    auto irlist = lower("has_args(a : 1, b : '2')");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));

    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_EQ(ir.name, "has_args");

    const auto & arguments = ir.pos_args;
    ASSERT_TRUE(arguments.empty());

    auto & kwargs = ir.kw_args;
    ASSERT_EQ(kwargs.size(), 2);

    const auto & kw_a = std::get<MIR::Number>(*kwargs.at("a").obj_ptr);
    ASSERT_EQ(kw_a.value, 1);

    const auto & kw_b = std::get<MIR::String>(*kwargs.at("b").obj_ptr);
    ASSERT_EQ(kw_b.value, "2");
}

TEST(ast_to_ir, function_both_arguments) {
    auto irlist = lower("both_args(1, a, a : 1)");

    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));

    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_EQ(ir.name, "both_args");

    const auto & arguments = ir.pos_args;
    ASSERT_EQ(arguments.size(), 2);
    ASSERT_EQ(std::get<MIR::Number>(*arguments[0].obj_ptr).value, 1);
    ASSERT_EQ(std::get<MIR::Identifier>(*arguments[1].obj_ptr).value, "a");

    auto & kwargs = ir.kw_args;
    ASSERT_EQ(kwargs.size(), 1);
    const auto & kw_a = std::get<MIR::Number>(*kwargs.at("a").obj_ptr);
    ASSERT_EQ(kw_a.value, 1);
}

TEST(ast_to_ir, if_only) {
    auto node = lower("if true\n 7\nendif\n");

    ASSERT_EQ(node->block->instructions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<MIR::Branch>(*node->block->instructions.back().obj_ptr));
    ASSERT_EQ(node->successors.size(), 2);
    ASSERT_TRUE(node->predecessors.empty());

    auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    ASSERT_EQ(branch.branches.size(), 2);

    auto & con = std::get<MIR::Boolean>(*std::get<0>(branch.branches.at(0)).obj_ptr);
    ASSERT_EQ(con.value, true);

    auto target = std::get<1>(branch.branches.at(0));
    ASSERT_EQ(target->successors.size(), 1);
    ASSERT_TRUE(node->successors.find(target) != node->successors.end());
    ASSERT_EQ(target->predecessors.size(), 1);
    ASSERT_TRUE(target->predecessors.find(node) != target->predecessors.end());

    ASSERT_EQ(target->block->instructions.size(), 2);
    ASSERT_TRUE(std::holds_alternative<MIR::Number>(*target->block->instructions.front().obj_ptr));
    ASSERT_EQ(std::get<MIR::Number>(*target->block->instructions.front().obj_ptr).value, 7);
    ASSERT_TRUE(std::holds_alternative<MIR::Jump>(*target->block->instructions.back().obj_ptr));

    auto fin = std::get<1>(branch.branches.at(1));
    ASSERT_EQ(fin->predecessors.size(), 2);
    ASSERT_TRUE(fin->successors.empty());
    ASSERT_TRUE(fin->block->instructions.empty());
    ASSERT_TRUE(fin->predecessors.find(node) != fin->predecessors.end());
    ASSERT_TRUE(fin->predecessors.find(target) != fin->predecessors.end());
    ASSERT_TRUE(node->successors.find(fin) != node->successors.end());
    ASSERT_TRUE(target->successors.find(fin) != target->successors.end());
}

TEST(ast_to_ir, if_branch_join) {
    auto node = lower(R"EOF(
        if true
          7
        endif
        8
        )EOF");

    EXPECT_EQ(node->successors.size(), 2);
    EXPECT_EQ(node->block->instructions.size(), 1);

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.front().obj_ptr);
    EXPECT_EQ(branch.branches.size(), 2);

    const auto & if_node = std::get<1>(branch.branches.at(0));
    EXPECT_EQ(if_node->predecessors.size(), 1);
    EXPECT_TRUE(node->successors.find(if_node) != node->successors.end());
    EXPECT_TRUE(if_node->predecessors.find(node) != if_node->predecessors.end());
    EXPECT_EQ(if_node->successors.size(), 1);
    EXPECT_EQ(if_node->block->instructions.size(), 2);
    EXPECT_EQ(std::get<MIR::Number>(*if_node->block->instructions.front().obj_ptr).value, 7);

    const auto & fin_node = std::get<1>(branch.branches.at(1));
    EXPECT_EQ(fin_node->predecessors.size(), 2);
    EXPECT_TRUE(node->successors.find(fin_node) != node->successors.end());
    EXPECT_TRUE(fin_node->predecessors.find(node) != fin_node->predecessors.end());
    EXPECT_TRUE(fin_node->predecessors.find(if_node) != fin_node->predecessors.end());
    EXPECT_EQ(fin_node->successors.size(), 0);
    EXPECT_EQ(fin_node->block->instructions.size(), 1);
    EXPECT_EQ(std::get<MIR::Number>(*fin_node->block->instructions.front().obj_ptr).value, 8);
}

TEST(ast_to_ir, if_else_more) {
    // Here we're testing that the jump value of both branches are the same, and
    // not nullptr. We should get one instruction in each branch, pluse one
    // instructino in the before and after blocks.
    auto node = lower(R"EOF(
        y = 0
        if true
          x = 7
        else
          x = 8
        endif
        y = x
        )EOF");

    EXPECT_EQ(node->predecessors.size(), 0);
    EXPECT_EQ(node->successors.size(), 2);

    EXPECT_EQ(node->block->instructions.size(), 2);
    EXPECT_TRUE(std::holds_alternative<MIR::Number>(*node->block->instructions.front().obj_ptr));
    EXPECT_TRUE(std::holds_alternative<MIR::Branch>(*node->block->instructions.back().obj_ptr));

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    EXPECT_EQ(branch.branches.size(), 2);

    const auto & if_node = std::get<1>(branch.branches.at(0));
    const auto & el_node = std::get<1>(branch.branches.at(1));

    EXPECT_TRUE(node->successors.find(if_node) != node->successors.end());
    EXPECT_TRUE(node->successors.find(el_node) != node->successors.end());
    EXPECT_EQ(if_node->predecessors.size(), 1);
    EXPECT_TRUE(if_node->predecessors.find(node) != if_node->predecessors.end());
    EXPECT_EQ(el_node->predecessors.size(), 1);
    EXPECT_TRUE(el_node->predecessors.find(node) != el_node->predecessors.end());

    EXPECT_EQ(if_node->block->instructions.size(), 2);
    EXPECT_TRUE(std::holds_alternative<MIR::Number>(*if_node->block->instructions.front().obj_ptr));
    EXPECT_TRUE(std::holds_alternative<MIR::Jump>(*if_node->block->instructions.back().obj_ptr));
    EXPECT_EQ(if_node->block->instructions.size(), 2);
    EXPECT_TRUE(std::holds_alternative<MIR::Number>(*if_node->block->instructions.front().obj_ptr));
    EXPECT_TRUE(std::holds_alternative<MIR::Jump>(*if_node->block->instructions.back().obj_ptr));
    EXPECT_EQ(std::get<MIR::Number>(*if_node->block->instructions.front().obj_ptr).value, 7);

    EXPECT_EQ(el_node->block->instructions.size(), 2);
    EXPECT_TRUE(std::holds_alternative<MIR::Number>(*el_node->block->instructions.front().obj_ptr));
    EXPECT_TRUE(std::holds_alternative<MIR::Jump>(*el_node->block->instructions.back().obj_ptr));
    EXPECT_EQ(std::get<MIR::Number>(*el_node->block->instructions.front().obj_ptr).value, 8);

    auto & tail = std::get<MIR::Jump>(*el_node->block->instructions.back().obj_ptr).target;
    EXPECT_EQ(tail->predecessors.size(), 2);
    EXPECT_EQ(tail->successors.size(), 0);
    EXPECT_TRUE(tail->predecessors.find(if_node) != tail->predecessors.end());
    EXPECT_TRUE(tail->predecessors.find(el_node) != tail->predecessors.end());
    EXPECT_TRUE(if_node->successors.find(tail) != if_node->successors.end());
    EXPECT_TRUE(el_node->successors.find(tail) != el_node->successors.end());
    EXPECT_EQ(tail->block->instructions.size(), 1);
    EXPECT_EQ(std::get<MIR::Identifier>(*tail->block->instructions.front().obj_ptr).value, "x");
}

TEST(ast_to_ir, if_elif_else_more) {
    auto node = lower(R"EOF(
        y = 0     # 0
        if true
          x = 7   # 1
        elif false
          x = 9   # 2
        else
          x = 8   # 3
        endif
        y = x     # 4
        )EOF");

    EXPECT_EQ(node->block->instructions.size(), 2);
    EXPECT_EQ(node->successors.size(), 3);
    EXPECT_EQ(node->predecessors.size(), 0);

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    ASSERT_EQ(branch.branches.size(), node->successors.size());

    const auto & sub = std::get<1>(branch.branches.at(0));
    const auto & tail = std::get<MIR::Jump>(*sub->block->instructions.back().obj_ptr).target;
    EXPECT_EQ(tail->block->instructions.size(), 1);
    ASSERT_EQ(std::get<MIR::Identifier>(*tail->block->instructions.front().obj_ptr).value, "x");
    EXPECT_EQ(tail->successors.size(), 0);
    EXPECT_EQ(tail->predecessors.size(), 3);

    for (auto && [_, s] : branch.branches) {
        EXPECT_EQ(s->predecessors.size(), 1);
        EXPECT_NE(s->predecessors.find(node), s->predecessors.end());
        EXPECT_NE(node->successors.find(s), node->successors.end());
        EXPECT_EQ(s->successors.size(), 1);
        EXPECT_NE(s->successors.find(tail), s->successors.end());
        EXPECT_NE(tail->predecessors.find(s), tail->predecessors.end());
    }
}

TEST(ast_to_ir, if_else) {
    auto node = lower("if true\n 7\nelse\n8\nendif\n");

    EXPECT_EQ(node->block->instructions.size(), 1);
    EXPECT_EQ(node->successors.size(), 2);
    EXPECT_EQ(node->predecessors.size(), 0);

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    ASSERT_EQ(branch.branches.size(), node->successors.size());

    const auto & sub = std::get<1>(branch.branches.at(0));
    const auto & tail = std::get<MIR::Jump>(*sub->block->instructions.back().obj_ptr).target;
    EXPECT_EQ(tail->block->instructions.size(), 0);
    EXPECT_EQ(tail->successors.size(), 0);
    EXPECT_EQ(tail->predecessors.size(), 2);

    for (auto && [_, s] : branch.branches) {
        EXPECT_EQ(s->predecessors.size(), 1);
        EXPECT_NE(s->predecessors.find(node), s->predecessors.end());
        EXPECT_NE(node->successors.find(s), node->successors.end());
        EXPECT_EQ(s->successors.size(), 1);
        EXPECT_NE(s->successors.find(tail), s->successors.end());
        EXPECT_NE(tail->predecessors.find(s), tail->predecessors.end());
    }
}

TEST(ast_to_ir, if_elif) {
    auto node = lower(R"EOF(
        if true
          7
        elif false
          8
        elif true
          9
        endif
        )EOF");
    EXPECT_EQ(node->block->instructions.size(), 1);
    EXPECT_EQ(node->successors.size(), 4);
    EXPECT_EQ(node->predecessors.size(), 0);

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    ASSERT_EQ(branch.branches.size(), node->successors.size());

    const auto & tail = std::get<1>(branch.branches.back());
    EXPECT_EQ(tail->block->instructions.size(), 0);
    EXPECT_EQ(tail->successors.size(), 0);
    EXPECT_EQ(tail->predecessors.size(), 4);

    for (auto && [_, s] : branch.branches) {
        if (s->index == tail->index) {
            continue;
        }
        EXPECT_EQ(s->predecessors.size(), 1);
        EXPECT_NE(s->predecessors.find(node), s->predecessors.end());
        EXPECT_NE(node->successors.find(s), node->successors.end());
        EXPECT_EQ(s->successors.size(), 1);
        EXPECT_NE(s->successors.find(tail), s->successors.end());
        EXPECT_NE(tail->predecessors.find(s), tail->predecessors.end());
    }
}

TEST(ast_to_ir, if_elif_else) {
    auto node = lower(R"EOF(
                   # 0
        if true
            7      # 1
        elif false
            8      # 2
        else
            9      # 3
        endif
        22         # 4
    )EOF");

    EXPECT_EQ(node->block->instructions.size(), 1);
    EXPECT_EQ(node->successors.size(), 3);
    EXPECT_EQ(node->predecessors.size(), 0);

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    ASSERT_EQ(branch.branches.size(), node->successors.size());

    const auto & sub = std::get<1>(branch.branches.at(0));
    const auto & tail = std::get<MIR::Jump>(*sub->block->instructions.back().obj_ptr).target;
    EXPECT_EQ(tail->block->instructions.size(), 1);
    EXPECT_EQ(std::get<MIR::Number>(*tail->block->instructions.front().obj_ptr).value, 22);
    EXPECT_EQ(tail->successors.size(), 0);
    EXPECT_EQ(tail->predecessors.size(), 3);

    for (auto && [_, s] : branch.branches) {
        EXPECT_EQ(s->predecessors.size(), 1);
        EXPECT_NE(s->predecessors.find(node), s->predecessors.end());
        EXPECT_NE(node->successors.find(s), node->successors.end());
        EXPECT_EQ(s->successors.size(), 1);
        EXPECT_NE(s->successors.find(tail), s->successors.end());
        EXPECT_NE(tail->predecessors.find(s), tail->predecessors.end());
    }
}

TEST(ast_to_ir, nested_if) {
    auto node = lower(R"EOF(
        if true
            7
            if false
                10
            endif
        elif false
            8
        else
            9
        endif
        22
    )EOF");

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    EXPECT_EQ(node->successors.size(), 3);
    ASSERT_EQ(branch.branches.size(), node->successors.size());

    const auto & nested = std::get<1>(branch.branches.at(0));
    EXPECT_EQ(nested->successors.size(), 2);
    ASSERT_TRUE(std::holds_alternative<MIR::Branch>(*nested->block->instructions.back().obj_ptr));
}

TEST(ast_to_ir, nested_if_tail) {
    auto node = lower(R"EOF(
        99               # 1
        if true
            7            # 2
            if false
                10       # 3
            endif
            12           # 4
        endif
        22               # 5
    )EOF");

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    EXPECT_EQ(node->successors.size(), 2);
    ASSERT_EQ(branch.branches.size(), node->successors.size());

    const auto & nested = std::get<1>(branch.branches.at(0));
    EXPECT_EQ(nested->successors.size(), 2);
    ASSERT_TRUE(std::holds_alternative<MIR::Branch>(*nested->block->instructions.back().obj_ptr));
}

TEST(ast_to_ir, nested_if_no_tail) {
    auto node = lower(R"EOF(
        99               # 1
        if true
            7            # 2
            if false
                10       # 3
            endif
            12           # 4
        endif
        # 5
    )EOF");

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    EXPECT_EQ(node->successors.size(), 2);
    ASSERT_EQ(branch.branches.size(), node->successors.size());

    const auto & nested = std::get<1>(branch.branches.at(0));
    EXPECT_EQ(nested->successors.size(), 2);
    ASSERT_TRUE(std::holds_alternative<MIR::Branch>(*nested->block->instructions.back().obj_ptr));
}

TEST(ast_to_ir, nested_if_elif_tail) {
    auto node = lower(R"EOF(
        x = 7      # 0
        if false
          x = 8    # 1
        elif A
          x = 16   # 2
        else
                   # 3
          if Q
            y = 7  # 4
          else
            y = 9  # 5
          endif
                   # 6
          if X
            x = 99 # 7
          endif
          x = 9    # 8
        endif
        y = x
        z = y      # 9
        )EOF");

    const auto & branch = std::get<MIR::Branch>(*node->block->instructions.back().obj_ptr);
    EXPECT_EQ(node->successors.size(), 3);
    ASSERT_EQ(branch.branches.size(), node->successors.size());

    const auto & nested = std::get<1>(branch.branches.at(0));
    EXPECT_EQ(nested->successors.size(), 1);
}

TEST(ast_to_ir, assign) {
    auto irlist = lower("a = 5");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_EQ(obj.var.name, "a");
    ASSERT_EQ(obj.var.gvn, 0);

    ASSERT_TRUE(std::holds_alternative<MIR::Number>(*obj.obj_ptr));
    const auto & ir = std::get<MIR::Number>(*obj.obj_ptr);
    ASSERT_EQ(ir.value, 5);
}

TEST(ast_to_ir, assign_from_id) {
    auto irlist = lower("a = b");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_EQ(obj.var.name, "a");
    ASSERT_EQ(obj.var.gvn, 0);

    ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*obj.obj_ptr));
    const auto & ir = std::get<MIR::Identifier>(*obj.obj_ptr);
    ASSERT_EQ(ir.value, "b");
}

TEST(ast_to_ir, not_simple) {
    auto irlist = lower("not true");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));
    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_EQ(ir.name, "unary_not");
    ASSERT_EQ(std::get<MIR::Boolean>(*ir.pos_args[0].obj_ptr).value, true);
}

TEST(ast_to_ir, not_method) {
    auto irlist = lower("not x.method()");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));
    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*ir.pos_args[0].obj_ptr));
}

TEST(ast_to_ir, neg_simple) {
    auto irlist = lower("-5");
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr));
    const auto & ir = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    ASSERT_EQ(ir.name, "unary_neg");
    ASSERT_EQ(std::get<MIR::Number>(*ir.pos_args[0].obj_ptr).value, 5);
}
