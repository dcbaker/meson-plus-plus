// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

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

MIR::BasicBlock lower(const std::string & in) {
    auto block = parse(in);
    const MIR::State::Persistant pstate{"foo/src", "foo/build"};
    auto ir = MIR::lower_ast(block, pstate);
    return ir;
}

inline bool is_bb(const MIR::NextType & next) {
    return std::holds_alternative<std::shared_ptr<MIR::BasicBlock>>(next);
}

inline std::shared_ptr<MIR::BasicBlock> get_bb(const MIR::NextType & next) {
    return std::get<std::shared_ptr<MIR::BasicBlock>>(next);
}

inline bool is_con(const MIR::NextType & next) {
    return std::holds_alternative<std::unique_ptr<MIR::Condition>>(next);
}

inline const std::unique_ptr<MIR::Condition> & get_con(const MIR::NextType & next) {
    return std::get<std::unique_ptr<MIR::Condition>>(next);
}

inline bool is_empty(const MIR::NextType & next) {
    return std::holds_alternative<std::monostate>(next);
}

} // namespace

TEST(ast_to_ir, number) {
    auto irlist = lower("7");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(obj));
    const auto & ir = std::get<std::unique_ptr<MIR::Number>>(obj);
    ASSERT_EQ(ir->value, 7);
}

TEST(ast_to_ir, boolean) {
    auto irlist = lower("true");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(obj));
    const auto & ir = std::get<std::unique_ptr<MIR::Boolean>>(obj);
    ASSERT_EQ(ir->value, true);
}

TEST(ast_to_ir, string) {
    auto irlist = lower("'true'");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::String>>(obj));
    const auto & ir = std::get<std::unique_ptr<MIR::String>>(obj);
    ASSERT_EQ(ir->value, "true");
}

TEST(ast_to_ir, array) {
    auto irlist = lower("['a', 'b', 1, [2]]");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Array>>(obj));

    const auto & arr = std::get<std::unique_ptr<MIR::Array>>(obj);

    const auto & arr0 = std::get<std::unique_ptr<MIR::String>>(arr->value[0]);
    ASSERT_EQ(arr0->value, "a");

    const auto & arr1 = std::get<std::unique_ptr<MIR::String>>(arr->value[1]);
    ASSERT_EQ(arr1->value, "b");

    const auto & arr2 = std::get<std::unique_ptr<MIR::Number>>(arr->value[2]);
    ASSERT_EQ(arr2->value, 1);

    const auto & arr3 = std::get<std::unique_ptr<MIR::Array>>(arr->value[3]);
    const auto & arr3_1 = std::get<std::unique_ptr<MIR::Number>>(arr3->value[0]);
    ASSERT_EQ(arr3_1->value, 2);
}

TEST(ast_to_ir, dict) {
    auto irlist = lower("{'str': 1}");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Dict>>(obj));

    const auto & dict = std::get<std::unique_ptr<MIR::Dict>>(obj);

    const auto & val = std::get<std::unique_ptr<MIR::Number>>(dict->value["str"]);
    ASSERT_EQ(val->value, 1);
}

TEST(ast_to_ir, simple_function) {
    auto irlist = lower("has_no_args()");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(obj));

    const auto & ir = std::get<std::unique_ptr<MIR::FunctionCall>>(obj);
    ASSERT_EQ(ir->name, "has_no_args");
    const auto & arguments = ir->pos_args;
    ASSERT_TRUE(arguments.empty());
}

TEST(ast_to_ir, simple_method) {
    auto irlist = lower("obj.method()");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(obj));

    const auto & ir = std::get<std::unique_ptr<MIR::FunctionCall>>(obj);
    ASSERT_EQ(ir->name, "method");
    ASSERT_EQ(ir->source_dir, ""); // We want to ensure this isn't meson.build
    ASSERT_TRUE(ir->holder.has_value());
    ASSERT_EQ(ir->holder.value(), "obj");
    ASSERT_TRUE(ir->pos_args.empty());
    ASSERT_TRUE(ir->kw_args.empty());
}

// TODO: chaning methods doesn't work
// To be fair it's pretty rare to do this, but it does happen.
#if 0
TEST(ast_to_ir, chained_method) {
    auto irlist = lower("obj.method().chained()");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(obj));

    const auto & ir = std::get<std::unique_ptr<MIR::FunctionCall>>(obj);
    ASSERT_EQ(ir->name, "method");
    ASSERT_TRUE(ir->holder.has_value());
    ASSERT_EQ(ir->holder.value(), "obj");
    ASSERT_TRUE(ir->pos_args.empty());
    ASSERT_TRUE(ir->kw_args.empty());
}
#endif

TEST(ast_to_ir, function_positional_arguments_only) {
    auto irlist = lower("has_args(1, 2, 3)");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(obj));

    const auto & ir = std::get<std::unique_ptr<MIR::FunctionCall>>(obj);
    ASSERT_EQ(ir->name, "has_args");

    const auto & arguments = ir->pos_args;
    ASSERT_EQ(arguments.size(), 3);
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(arguments[0])->value, 1);
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(arguments[1])->value, 2);
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(arguments[2])->value, 3);
}

TEST(ast_to_ir, function_keyword_arguments_only) {
    auto irlist = lower("has_args(a : 1, b : '2')");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(obj));

    const auto & ir = std::get<std::unique_ptr<MIR::FunctionCall>>(obj);
    ASSERT_EQ(ir->name, "has_args");

    const auto & arguments = ir->pos_args;
    ASSERT_TRUE(arguments.empty());

    auto & kwargs = ir->kw_args;
    ASSERT_EQ(kwargs.size(), 2);

    const auto & kw_a = std::get<std::unique_ptr<MIR::Number>>(kwargs["a"]);
    ASSERT_EQ(kw_a->value, 1);

    const auto & kw_b = std::get<std::unique_ptr<MIR::String>>(kwargs["b"]);
    ASSERT_EQ(kw_b->value, "2");
}

TEST(ast_to_ir, function_both_arguments) {
    auto irlist = lower("both_args(1, a, a : 1)");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(obj));

    const auto & ir = std::get<std::unique_ptr<MIR::FunctionCall>>(obj);
    ASSERT_EQ(ir->name, "both_args");

    const auto & arguments = ir->pos_args;
    ASSERT_EQ(arguments.size(), 2);
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(arguments[0])->value, 1);
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Identifier>>(arguments[1])->value, "a");

    auto & kwargs = ir->kw_args;
    ASSERT_EQ(kwargs.size(), 1);
    const auto & kw_a = std::get<std::unique_ptr<MIR::Number>>(kwargs["a"]);
    ASSERT_EQ(kw_a->value, 1);
}

TEST(ast_to_ir, if_only) {
    auto irlist = lower("if true\n 7\nendif\n");
    ASSERT_TRUE(is_con(irlist.next));
    auto const & con = get_con(irlist.next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(con->condition));

    auto const & if_true = con->if_true->instructions;
    ASSERT_EQ(if_true.size(), 1);

    auto const & val = if_true.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 7);
}

TEST(ast_to_ir, if_branch_join) {
    auto irlist = lower(R"EOF(
        if true
          7
        endif
        8
        )EOF");
    ASSERT_TRUE(is_con(irlist.next));
    auto const & con = get_con(irlist.next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(con->condition));

    auto const & if_true = con->if_true->instructions;
    ASSERT_EQ(if_true.size(), 1);

    auto const & val = if_true.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 7);

    ASSERT_NE(con->if_false, nullptr);

    auto const & block2 = con->if_false;
    ASSERT_NE(block2, nullptr);
    auto const & val2 = block2->instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val2));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val2)->value, 8);

    ASSERT_EQ(get_bb(con->if_true->next), block2);
}

TEST(ast_to_ir, if_else_more) {
    // Here we're testing that the jupm value of both branches are the same, and
    // not nullptr. We should get one instruction in each branch, pluse one
    // instructino in the before and after blocks.
    auto irlist = lower(R"EOF(
        y = 0
        if true
          x = 7
        else
          x = 8
        endif
        y = x
        )EOF");

    ASSERT_EQ(irlist.instructions.size(), 1);
    ASSERT_TRUE(is_con(irlist.next));
    auto const & con = get_con(irlist.next);

    ASSERT_EQ(con->if_true->instructions.size(), 1);
    ASSERT_TRUE(con->if_true->parents.count(&irlist));
    ASSERT_EQ(con->if_false->instructions.size(), 1);
    ASSERT_TRUE(con->if_false->parents.count(&irlist));

    ASSERT_TRUE(is_bb(con->if_true->next));
    ASSERT_EQ(get_bb(con->if_true->next), get_bb(con->if_false->next));

    ASSERT_EQ(get_bb(con->if_true->next)->instructions.size(), 1);
}

TEST(ast_to_ir, if_elif_else_more) {
    auto irlist = lower(R"EOF(
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
    // block 0
    ASSERT_EQ(irlist.instructions.size(), 1);
    ASSERT_TRUE(is_con(irlist.next));

    // block 1
    auto const & con = get_con(irlist.next);
    ASSERT_EQ(con->if_true->instructions.size(), 1);
    ASSERT_TRUE(is_bb(con->if_true->next));

    const auto & out_block = get_bb(con->if_true->next);

    // block 2
    ASSERT_TRUE(is_con(con->if_false->next));
    auto const & con1 = get_con(con->if_false->next);
    ASSERT_EQ(con1->if_true->instructions.size(), 1);
    ASSERT_TRUE(is_bb(con1->if_true->next));

    // block 3
    auto const & else_block = con1->if_false;
    ASSERT_EQ(else_block->instructions.size(), 1);
    ASSERT_TRUE(is_bb(else_block->next));
}

TEST(ast_to_ir, if_else) {
    auto irlist = lower("if true\n 7\nelse\n8\nendif\n");
    ASSERT_TRUE(is_con(irlist.next));
    auto const & con = get_con(irlist.next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(con->condition));

    auto const & if_true = con->if_true->instructions;
    ASSERT_EQ(if_true.size(), 1);
    auto const & val = if_true.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 7);

    auto const & if_false = con->if_false->instructions;
    ASSERT_EQ(if_false.size(), 1);
    auto const & val2 = if_false.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val2));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val2)->value, 8);
}

TEST(ast_to_ir, if_elif) {
    auto irlist = lower(R"EOF(
        if true
          7
        elif false
          8
        elif true
          9
        endif
        )EOF");
    ASSERT_TRUE(is_con(irlist.next));
    auto const & con = get_con(irlist.next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(con->condition));

    {
        auto const & if_true = con->if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 7);
    }

    ASSERT_NE(con->if_false, nullptr);
    auto const & elcon = get_con(con->if_false->next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(elcon->condition));

    {
        auto const & if_true = elcon->if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 8);
    }

    auto const & elcon2 = get_con(elcon->if_false->next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(elcon2->condition));

    {
        auto const & if_true = elcon2->if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 9);
    }
}

TEST(ast_to_ir, if_elif_else) {
    auto irlist = lower(R"EOF(
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

    // block 0
    ASSERT_TRUE(is_con(irlist.next));
    auto const & con = get_con(irlist.next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(con->condition));

    // block 1
    {
        auto const & if_true = con->if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 7);
    }

    ASSERT_NE(con->if_false, nullptr);
    auto const & elcon = get_con(con->if_false->next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(elcon->condition));

    // /block 2
    auto const & if_true = elcon->if_true->instructions;
    ASSERT_EQ(if_true.size(), 1);
    auto const & val = if_true.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 8);

    // blcok 3
    auto const & if_false = elcon->if_false->instructions;
    ASSERT_EQ(if_false.size(), 1);
    auto const & val2 = if_false.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val2));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val2)->value, 9);
}

TEST(ast_to_ir, nested_if) {
    auto irlist = lower(R"EOF(
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
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Condition>>(irlist.next));
    auto const & con = std::get<std::unique_ptr<MIR::Condition>>(irlist.next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(con->condition));

    {
        auto const & if_true = con->if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 7);

        auto const & con2 = get_con(con->if_true->next);
        auto const & val2 = con2->if_true->instructions.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val2));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val2)->value, 10);

        const auto & fin = get_bb(get_bb(con2->if_true->next)->next);
        ASSERT_EQ(fin->instructions.size(), 1);
        auto const & val3 = fin->instructions.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val3));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val3)->value, 22);
    }

    ASSERT_NE(con->if_false, nullptr);
    auto const & elcon = get_con(con->if_false->next);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Boolean>>(elcon->condition));

    {
        auto const & if_true = elcon->if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val)->value, 8);

        auto const & if_false = elcon->if_false->instructions;
        ASSERT_EQ(if_false.size(), 1);
        auto const & val2 = if_false.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(val2));
        ASSERT_EQ(std::get<std::unique_ptr<MIR::Number>>(val2)->value, 9);
    }
}

TEST(ast_to_ir, nested_if_tail) {
    auto irlist = lower(R"EOF(
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

    ASSERT_TRUE(is_con(irlist.next));

    const auto & con1 = get_con(irlist.next);
    ASSERT_TRUE(is_con(con1->if_true->next));

    const auto & con2 = get_con(con1->if_true->next);
    ASSERT_TRUE(is_bb(con2->if_true->next));

    // block 3 and block 2 should both go to block 4
    ASSERT_EQ(get_bb(con2->if_true->next), con2->if_false);

    const auto & last_block = con1->if_false;

    // Block 4 and block 1 should both go to block 5
    ASSERT_EQ(con1->if_false, get_bb(con2->if_false->next));
    ASSERT_EQ(last_block->parents.size(), 2);
    ASSERT_TRUE(last_block->parents.count(con2->if_false.get()));
    ASSERT_TRUE(last_block->parents.count(&irlist));
}

TEST(ast_to_ir, nested_if_no_tail) {
    auto irlist = lower(R"EOF(
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

    ASSERT_TRUE(is_con(irlist.next));

    const auto & con1 = get_con(irlist.next);
    ASSERT_TRUE(is_con(con1->if_true->next));

    const auto & con2 = get_con(con1->if_true->next);
    ASSERT_TRUE(is_bb(con2->if_true->next));

    // block 3 and block 2 should both go to block 4
    ASSERT_EQ(get_bb(con2->if_true->next), con2->if_false);

    const auto & last_block = con1->if_false;

    // Block 4 and block 1 should both go to block 5
    ASSERT_EQ(con1->if_false, get_bb(con2->if_false->next));
    ASSERT_EQ(last_block->parents.size(), 2);
    ASSERT_TRUE(last_block->parents.count(con2->if_false.get()));
    ASSERT_TRUE(last_block->parents.count(&irlist));
}

TEST(ast_to_ir, nested_if_elif_tail) {
    auto irlist = lower(R"EOF(
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

    const auto & fin = get_bb(get_con(irlist.next)->if_true->next);

    // block 2 (elif A)
    const auto & con2 = get_con(irlist.next)->if_false;
    ASSERT_EQ(fin, get_bb(get_con(con2->next)->if_true->next));

    // block 3 (else)
    const auto & con3 = get_con(con2->next)->if_false;
    ASSERT_TRUE(is_con(con3->next));
    const auto & con31 = get_con(con3->next);
    const auto & bb6 = get_bb(con31->if_false->next);

    const auto & con32 = get_con(bb6->next);
    const auto & bb8 = get_bb(con32->if_false);

    ASSERT_EQ(get_bb(bb8->next), fin);
}

TEST(ast_to_ir, assign) {
    auto irlist = lower("a = 5");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Number>>(obj));
    const auto & ir = std::get<std::unique_ptr<MIR::Number>>(obj);
    ASSERT_EQ(ir->value, 5);
    ASSERT_EQ(ir->var.name, "a");
    ASSERT_EQ(ir->var.version, 0);
}

TEST(ast_to_ir, assign_from_id) {
    auto irlist = lower("a = b");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(obj));
    const auto & ir = std::get<std::unique_ptr<MIR::Identifier>>(obj);
    ASSERT_EQ(ir->value, "b");
    ASSERT_EQ(ir->var.name, "a");
    ASSERT_EQ(ir->var.version, 0);
}
