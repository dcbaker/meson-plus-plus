// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <variant>

#include "node.hpp"
#include "parser.yy.hpp"
#include "scanner.hpp"

static std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in) {
    std::istringstream stream{in};
    auto block = std::make_unique<Frontend::AST::CodeBlock>();
    auto scanner = std::make_unique<Frontend::Scanner>(&stream);
    auto parser = std::make_unique<Frontend::Parser>(*scanner, block);
    parser->parse();
    return block;
}

TEST(parser, string) {
    auto block = parse("'foo'");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::String>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "'foo'");
}

TEST(parser, decminal_number) {
    auto block = parse("77");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Number>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "77");
}

TEST(parser, octal_number) {
    auto block = parse("0o10");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Number>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "8");
}

TEST(parser, hex_number) {
    auto block = parse("0xf");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Number>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "15");
}

TEST(parser, identifier) {
    auto block = parse("foo");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Identifier>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "foo");
}

TEST(parser, multiplication) {
    auto block = parse("5  * 4 ");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::MultiplicativeExpression>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "5 * 4");
}

TEST(parser, division) {
    auto block = parse("5 / 4 ");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::MultiplicativeExpression>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "5 / 4");
}

TEST(parser, addition) {
    auto block = parse("5 + 4 ");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::AdditiveExpression>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "5 + 4");
}

TEST(parser, subtraction) {
    auto block = parse("5 - 4 ");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::AdditiveExpression>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "5 - 4");
}

TEST(parser, mod) {
    auto block = parse("5 % 4 ");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::MultiplicativeExpression>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "5 % 4");
}

TEST(parser, unary_negate) {
    auto block = parse("- 5");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::UnaryExpression>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "-5");
}

TEST(parser, assignment) {
    auto block = parse("x = 5 + 3");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Assignment>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "x = 5 + 3");
}

TEST(parser, subscript) {
    auto block = parse("foo[bar + 1]");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Subscript>>(block->expressions[0]));
    ASSERT_EQ(block->as_string(), "foo[bar + 1]");
}

TEST(parser, parens) {
    auto block = parse("(4 * (5 + 3))");
    ASSERT_EQ(block->expressions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::MultiplicativeExpression>>(block->expressions[0]));
    // ASSERT_EQ(block->as_string(), "(4 * (5 +3))");
}
