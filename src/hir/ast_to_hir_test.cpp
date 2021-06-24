// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>
#include <sstream>
#include <variant>

#include "ast_to_hir.hpp"
#include "driver.hpp"
#include "hir.hpp"

namespace {

std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in) {
    Frontend::Driver drv{};
    std::istringstream stream{in};
    drv.name = "test file name";
    auto block = drv.parse(stream);
    return block;
}

HIR::IRList lower(const std::string & in) {
    auto block = parse(in);
    auto ir = HIR::lower_ast(block);
    return ir;
}

} // namespace

TEST(ast_to_ir, number) {
    auto irlist = lower("7");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(obj));
    const auto & ir = std::get<std::unique_ptr<HIR::Number>>(obj);
    ASSERT_EQ(ir->value, 7);
}

TEST(ast_to_ir, boolean) {
    auto irlist = lower("true");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(obj));
    const auto & ir = std::get<std::unique_ptr<HIR::Boolean>>(obj);
    ASSERT_EQ(ir->value, true);
}

TEST(ast_to_ir, string) {
    auto irlist = lower("'true'");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::String>>(obj));
    const auto & ir = std::get<std::unique_ptr<HIR::String>>(obj);
    ASSERT_EQ(ir->value, "true");
}

TEST(ast_to_ir, array) {
    auto irlist = lower("['a', 'b', 1, [2]]");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Array>>(obj));

    const auto & arr = std::get<std::unique_ptr<HIR::Array>>(obj);

    const auto & arr0 = std::get<std::unique_ptr<HIR::String>>(arr->value[0]);
    ASSERT_EQ(arr0->value, "a");

    const auto & arr1 = std::get<std::unique_ptr<HIR::String>>(arr->value[1]);
    ASSERT_EQ(arr1->value, "b");

    const auto & arr2 = std::get<std::unique_ptr<HIR::Number>>(arr->value[2]);
    ASSERT_EQ(arr2->value, 1);

    const auto & arr3 = std::get<std::unique_ptr<HIR::Array>>(arr->value[3]);
    const auto & arr3_1 = std::get<std::unique_ptr<HIR::Number>>(arr3->value[0]);
    ASSERT_EQ(arr3_1->value, 2);
}

TEST(ast_to_ir, dict) {
    auto irlist = lower("{'str': 1}");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Dict>>(obj));

    const auto & dict = std::get<std::unique_ptr<HIR::Dict>>(obj);

    const auto & val = std::get<std::unique_ptr<HIR::Number>>(dict->value["str"]);
    ASSERT_EQ(val->value, 1);
}

TEST(ast_to_ir, simple_function) {
    auto irlist = lower("has_no_args()");
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::FunctionCall>>(obj));

    const auto & ir = std::get<std::unique_ptr<HIR::FunctionCall>>(obj);
    const auto & name = ir->name;
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Identifier>>(name));
    const auto & raw_name = std::move(std::get<std::unique_ptr<HIR::Identifier>>(name));
    ASSERT_EQ(raw_name->value, "has_no_args");
}

TEST(ast_to_ir, if_only) {
    auto irlist = lower("if true\n 7\nendif\n");
    ASSERT_TRUE(irlist.condition.has_value());
    auto const & con = irlist.condition.value();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(con.condition));

    auto const & if_true = con.if_true->instructions;
    ASSERT_EQ(if_true.size(), 1);

    auto const & val = if_true.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val));
    ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val)->value, 7);
}

TEST(ast_to_ir, if_else) {
    auto irlist = lower("if true\n 7\nelse\n8\nendif\n");
    ASSERT_TRUE(irlist.condition.has_value());
    auto const & con = irlist.condition.value();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(con.condition));

    auto const & if_true = con.if_true->instructions;
    ASSERT_EQ(if_true.size(), 1);
    auto const & val = if_true.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val));
    ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val)->value, 7);

    auto const & if_false = con.if_false->instructions;
    ASSERT_EQ(if_false.size(), 1);
    auto const & val2 = if_false.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val2));
    ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val2)->value, 8);
}

TEST(ast_to_ir, if_elif) {
    auto irlist = lower("if true\n 7\nelif false\n8\nelif true\n9\nendif\n");
    ASSERT_TRUE(irlist.condition.has_value());
    auto const & con = irlist.condition.value();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(con.condition));

    {
        auto const & if_true = con.if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val)->value, 7);

        auto const & if_false = con.if_false->instructions;
        ASSERT_EQ(if_false.size(), 0);
    }

    ASSERT_TRUE(con.if_false->condition.has_value());
    auto const & elcon = con.if_false->condition.value();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(elcon.condition));

    {
        auto const & if_true = elcon.if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val)->value, 8);

        auto const & if_false = elcon.if_false->instructions;
        ASSERT_EQ(if_false.size(), 0);
    }

    auto const & elcon2 = elcon.if_false->condition.value();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(elcon2.condition));

    {
        auto const & if_true = elcon2.if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val)->value, 9);

        auto const & if_false = elcon.if_false->instructions;
        ASSERT_EQ(if_false.size(), 0);
    }
}

TEST(ast_to_ir, if_elif_else) {
    auto irlist = lower("if true\n 7\nelif false\n8\nelse\n9\nendif\n");
    ASSERT_TRUE(irlist.condition.has_value());
    auto const & con = irlist.condition.value();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(con.condition));

    {
        auto const & if_true = con.if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val)->value, 7);
    }

    ASSERT_TRUE(con.if_false->condition.has_value());
    auto const & elcon = con.if_false->condition.value();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(elcon.condition));

    {
        auto const & if_true = elcon.if_true->instructions;
        ASSERT_EQ(if_true.size(), 1);
        auto const & val = if_true.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val));
        ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val)->value, 8);

        auto const & if_false = elcon.if_false->instructions;
        ASSERT_EQ(if_false.size(), 1);
        auto const & val2 = if_false.front();
        ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(val2));
        ASSERT_EQ(std::get<std::unique_ptr<HIR::Number>>(val2)->value, 9);
    }
}
