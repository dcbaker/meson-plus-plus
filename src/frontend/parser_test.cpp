// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <variant>

#include "driver.hpp"
#include "node.hpp"

static std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in) {
    Frontend::Driver drv{};
    std::istringstream stream{in};
    drv.name = "test file name";
    auto block = drv.parse(stream);
    return block;
}

TEST(parser, string) {
    auto block = parse("'foo'");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::String>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "'foo'");
}

TEST(parser, escape_in_string) {
    auto block = parse("'can\\'t'");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::String>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "'can't'");
}

TEST(parser, newline_in_string) {
    auto block = parse("'can\\'t\\nstop'");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::String>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "'can't\nstop'");
}

TEST(parser, tab_in_string) {
    auto block = parse("'\\ttab'");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::String>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "'\ttab'");
}

TEST(parser, backslash_in_string) {
    auto block = parse("'\\\\tab'");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::String>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "'\\tab'");
}

TEST(parser, triple_string) {
    auto block = parse("'''foo'''");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::String>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "'''foo'''");
}

TEST(parser, triple_string_newlines) {
    auto block = parse("'''\nfoo\n\nbar'''");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::String>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "'''\nfoo\n\nbar'''");
}

TEST(parser, decminal_number) {
    auto block = parse("77");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Number>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "77");
}

TEST(parser, locations) {
    auto block = parse("77");
    auto const & stmt = std::get<0>(block->statements[0]);
    auto const & expr = *std::get_if<std::unique_ptr<Frontend::AST::Number>>(&stmt->expr);
    ASSERT_EQ(expr->loc.column_start, 1);
    ASSERT_EQ(expr->loc.line_start, 1);
    ASSERT_EQ(expr->loc.column_end, 3);
    ASSERT_EQ(expr->loc.line_end, 1);
    std::string expected{"test file name"};
    ASSERT_EQ(expected, expr->loc.filename);
}

TEST(parser, octal_number) {
    auto block = parse("0o10");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Number>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "8");
}

TEST(parser, hex_number) {
    auto block = parse("0xf");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Number>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "15");
}

TEST(parser, identifier) {
    auto block = parse("foo");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Identifier>>(stmt->expr));
    ASSERT_EQ(stmt->as_string(), "foo");
}

TEST(parser, multiplication) {
    auto block = parse("5  * 4 ");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::MultiplicativeExpression>>(
        stmt->expr));
    ASSERT_EQ(block->as_string(), "5 * 4");
}

TEST(parser, division) {
    auto block = parse("5 / 4 ");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::MultiplicativeExpression>>(
        stmt->expr));
    ASSERT_EQ(block->as_string(), "5 / 4");
}

TEST(parser, addition) {
    auto block = parse("5 + 4 ");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<Frontend::AST::AdditiveExpression>>(stmt->expr));
    ASSERT_EQ(block->as_string(), "5 + 4");
}

TEST(parser, subtraction) {
    auto block = parse("5 - 4 ");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<Frontend::AST::AdditiveExpression>>(stmt->expr));
    ASSERT_EQ(block->as_string(), "5 - 4");
}

TEST(parser, mod) {
    auto block = parse("5 % 4 ");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::MultiplicativeExpression>>(
        stmt->expr));
    ASSERT_EQ(block->as_string(), "5 % 4");
}

TEST(parser, unary_negate) {
    auto block = parse("- 5");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<Frontend::AST::UnaryExpression>>(stmt->expr));
    ASSERT_EQ(block->as_string(), "-5");
}

TEST(parser, unary_not) {
    auto block = parse("not true");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<Frontend::AST::UnaryExpression>>(stmt->expr));
    ASSERT_EQ(block->as_string(), "not true");
}

TEST(parser, subscript) {
    auto block = parse("foo[bar + 1]");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Subscript>>(stmt->expr));
    ASSERT_EQ(block->as_string(), "foo[bar + 1]");
}

TEST(parser, subexpression) {
    auto block = parse("(4 * (5 + 3))");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::MultiplicativeExpression>>(
        stmt->expr));
    // ASSERT_EQ(block->as_string(), "(4 * (5 +3))");
};

TEST(Parser, ternary) {
    auto block = parse("true ? x : b");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Ternary>>(stmt->expr));
    ASSERT_EQ(block->as_string(), "true ? x : b");
}

class RelationalToStringTests
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(RelationalToStringTests, relational) {
    const auto & [input, expected] = GetParam();
    auto block = parse(input);
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Relational>>(stmt->expr));
    ASSERT_EQ(block->as_string(), expected);
}
INSTANTIATE_TEST_CASE_P(RelationalParsingTests, RelationalToStringTests,
                        ::testing::Values(std::make_tuple("4<3", "4 < 3"),
                                          std::make_tuple("4>3", "4 > 3"),
                                          std::make_tuple("0 == true", "0 == true"),
                                          std::make_tuple("0 != true", "0 != true"),
                                          std::make_tuple("x or y", "x or y"),
                                          std::make_tuple("x and y", "x and y"),
                                          std::make_tuple("x in y", "x in y"),
                                          std::make_tuple("x not in y", "x not in y")));

class FunctionToStringTests
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(FunctionToStringTests, arguments) {
    const std::string input = std::get<0>(GetParam());
    const std::string expected = std::get<1>(GetParam());
    auto block = parse(input);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::FunctionCall>>(stmt->expr));
    ASSERT_EQ(block->as_string(), expected);
}

INSTANTIATE_TEST_CASE_P(
    FunctionParsingTests, FunctionToStringTests,
    ::testing::Values(std::make_tuple("func(  )", "func()"), std::make_tuple("func(a)", "func(a)"),
                      std::make_tuple("func(a,b, c)", "func(a, b, c)"),
                      std::make_tuple("func(a,)", "func(a)"),
                      std::make_tuple("func(x : 'f')", "func(x : 'f')"),
                      std::make_tuple("func(x : 'f', y : 1)", "func(x : 'f', y : 1)"),
                      std::make_tuple("func(a, b, x : 'f')", "func(a, b, x : 'f')"),
                      std::make_tuple("func(a,\nb,\nc)", "func(a, b, c)"),
                      std::make_tuple("func(a,\nb,\nc\n)", "func(a, b, c)"),
                      std::make_tuple("func(a : 1,\nb: 2,\nc : 3)", "func(a : 1, b : 2, c : 3)"),
                      std::make_tuple("func(a : 1,\nb: 2,\nc : 3\n)", "func(a : 1, b : 2, c : 3)"),
                      std::make_tuple("func(a,\nb,\nc : 1,\n d: 3)", "func(a, b, c : 1, d : 3)"),
                      std::make_tuple("func(a,\nb,\nc : 1,\n d: 3\n)",
                                      "func(a, b, c : 1, d : 3)")));

class MethodToStringTests : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
};

TEST_P(MethodToStringTests, arguments) {
    const auto & [input, expected] = GetParam();
    auto block = parse(input);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::GetAttribute>>(stmt->expr));
    ASSERT_EQ(block->as_string(), expected);
}

INSTANTIATE_TEST_CASE_P(
    MethodParsingTests, MethodToStringTests,
    ::testing::Values(
        std::make_tuple("o.m()", "o.m()"),
        std::make_tuple("meson.get_compiler ( 'cpp' )", "meson.get_compiler('cpp')"),
        std::make_tuple("meson.get_compiler ( 'cpp', 'c' )", "meson.get_compiler('cpp', 'c')"),
        std::make_tuple("o.method(x : y, z : 1)", "o.method(x : y, z : 1)"),
        std::make_tuple("o.method(a, b, x : y, z : 1)", "o.method(a, b, x : y, z : 1)")));

class ArrayToStringTests : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(ArrayToStringTests, arguments) {
    const auto & [input, expected] = GetParam();
    auto block = parse(input);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Array>>(stmt->expr));
    ASSERT_EQ(block->as_string(), expected);
}

INSTANTIATE_TEST_CASE_P(ArrayParsingTests, ArrayToStringTests,
                        ::testing::Values(std::make_tuple("[ ]", "[]"),
                                          std::make_tuple("[a, b]", "[a, b]"),
                                          std::make_tuple("[a, [b]]", "[a, [b]]"),
                                          std::make_tuple("[a, ]", "[a]"),
                                          std::make_tuple("[\n  a,\n  b\n]", "[a, b]")));

class DictToStringTests : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(DictToStringTests, arguments) {
    const auto & [input, expected] = GetParam();
    auto block = parse(input);
    auto const & stmt = std::get<0>(block->statements[0]);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Dict>>(stmt->expr));
    ASSERT_EQ(block->as_string(), expected);
}

INSTANTIATE_TEST_CASE_P(DictParsingTests, DictToStringTests,
                        ::testing::Values(std::make_tuple("{}", "{}"),
                                          std::make_tuple("{a:b}", "{a : b}"),
                                          std::make_tuple("{a : b, }", "{a : b}"),
                                          std::make_tuple("{a : b}", "{a : b}"),
                                          std::make_tuple("{'a' : 'b'}", "{'a' : 'b'}"),
                                          std::make_tuple("{'a' : func()}", "{'a' : func()}"),
                                          std::make_tuple("{a : [b]}", "{a : [b]}")));
// We can't test a multi item dict reliably like this be
// cause meson dicts are unordered

class AssignmentStatementParsingTests
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(AssignmentStatementParsingTests, arguments) {
    auto const & [input, expected] = GetParam();
    auto block = parse(input);
    auto const & stmt = block->statements[0];
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Assignment>>(stmt));
    ASSERT_EQ(block->as_string(), expected);
}

INSTANTIATE_TEST_CASE_P(
    parser, AssignmentStatementParsingTests,
    ::testing::Values(std::make_tuple("a=1+1", "a = 1 + 1"), std::make_tuple("a += 2", "a += 2"),
                      std::make_tuple("a -= 2", "a -= 2"), std::make_tuple("a *= 2", "a *= 2"),
                      std::make_tuple("a /= 2", "a /= 2"), std::make_tuple("a %= 2", "a %= 2")));

class IfStatementParsingTests : public ::testing::TestWithParam<std::string> {};

TEST_P(IfStatementParsingTests, arguments) {
    auto const & input = GetParam();
    auto block = parse(input);
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = block->statements[0];
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::IfStatement>>(stmt));
}

INSTANTIATE_TEST_CASE_P(
    parser, IfStatementParsingTests,
    ::testing::Values("if true\na = b\nendif", "if true\na = b\n\n\nendif",
                      "if false\na = b\nelse\na = c\nendif",
                      "if false\na = b\nelif true\na = c\nendif",
                      "if false\na = b\nelif false\na =b\nelif true\na = c\nendif",
                      "if false\na = b\nelif 1 == 2\na = c\nelse\na = d\nendif",
                      "if true\nif true\na = b\nendif\nendif"));

TEST(IfStatementParsingTests, multiple_if_body_statements) {
    auto block = parse("if true\na = b\ne = 1\nendif");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<std::unique_ptr<Frontend::AST::IfStatement>>(block->statements[0]);
    ASSERT_EQ(stmt->ifblock.block->statements.size(), 2);
}

TEST(IfStatementParsingTests, multiple_elif_body_statements) {
    auto block = parse("if true\na = b\ne = 1\nelif false\na = 2\nb = 3\n c = 4\nendif");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<std::unique_ptr<Frontend::AST::IfStatement>>(block->statements[0]);
    ASSERT_EQ(stmt->ifblock.block->statements.size(), 2);
    ASSERT_EQ(stmt->efblock.size(), 1);
    ASSERT_EQ(stmt->efblock[0].block->statements.size(), 3);
}

TEST(IfStatementParsingTests, multiple_elif_body_statements2) {
    auto block = parse(
        "if true\na = b\ne = 1\nelif false\na = 2\nb = 3\n c = 4\n\nelif 0\na = 1\nb = 1\nendif");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<std::unique_ptr<Frontend::AST::IfStatement>>(block->statements[0]);
    ASSERT_EQ(stmt->ifblock.block->statements.size(), 2);
    ASSERT_EQ(stmt->efblock.size(), 2);
    ASSERT_EQ(stmt->efblock[0].block->statements.size(), 3);
    ASSERT_EQ(stmt->efblock[1].block->statements.size(), 2);
}

TEST(IfStatementParsingTests, multiple_else_body_statements) {
    auto block = parse("if true\na = b\ne = 1\nelse\na = 2\nb = 3\n c = 4\nendif");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<std::unique_ptr<Frontend::AST::IfStatement>>(block->statements[0]);
    ASSERT_EQ(stmt->ifblock.block->statements.size(), 2);
    ASSERT_EQ(stmt->efblock.size(), 0);
    ASSERT_EQ(stmt->eblock.block->statements.size(), 3);
}

TEST(IfStatementParsingTests, multiple_elif_else_body_statements) {
    auto block = parse("if true\na = b\ne = 1\n"
                       "elif 1\na = b\nc = 2\n"
                       "elif 2\nd = 1\na = 2\nc = b\n"
                       "else\na = 2\nb = 3\n c = 4\nendif");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = std::get<std::unique_ptr<Frontend::AST::IfStatement>>(block->statements[0]);
    ASSERT_EQ(stmt->ifblock.block->statements.size(), 2);
    ASSERT_EQ(stmt->efblock.size(), 2);
    ASSERT_EQ(stmt->efblock[0].block->statements.size(), 2);
    ASSERT_EQ(stmt->efblock[1].block->statements.size(), 3);
    ASSERT_EQ(stmt->eblock.block->statements.size(), 3);
}
TEST(IfStatementParsingTests, back_to_back_if_statments) {
    auto block = parse("if true\na = 1\nendif\nif false\nb = 2\nendif\n");
    ASSERT_EQ(block->statements.size(), 2);
}

TEST(IfStatementParsingTests, backslash) {
    auto block = parse("if true\\\n  or false\na = 1\nendif\nif false\nb = 2\nendif\n");
    ASSERT_EQ(block->statements.size(), 2);
}

TEST(parser, foreach_statement) {
    auto block = parse("foreach x : a\na = b\ntarget()\nendforeach");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = block->statements[0];
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::ForeachStatement>>(stmt));
}

TEST(parser, foreach_statement_list) {
    auto block = parse("foreach x : ['a', 'b']\na = b\ntarget()\nendforeach");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = block->statements[0];
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::ForeachStatement>>(stmt));
}

TEST(parser, foreach_statement_dict) {
    auto block = parse("foreach k, v : {a : 'b', b : 1}\na = b\ntarget()\nendforeach");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = block->statements[0];
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::ForeachStatement>>(stmt));
}

TEST(parser, break_statement) {
    auto block = parse("break");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = block->statements[0];
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Break>>(stmt));
}

TEST(parser, continue_statement) {
    auto block = parse("continue");
    ASSERT_EQ(block->statements.size(), 1);
    auto const & stmt = block->statements[0];
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<Frontend::AST::Continue>>(stmt));
}

TEST(parser, trailing_newline) {
    auto block = parse("'foo'\n");
    ASSERT_EQ(block->statements.size(), 1);
}

TEST(parser, newline_in_statements) {
    auto block = parse("a = b\nb = c\n\n\nc = a\n");
    ASSERT_EQ(block->statements.size(), 3);
}

TEST(parser, comment_no_newline) {
    auto block = parse("a = 1\n  # foo");
    ASSERT_EQ(block->statements.size(), 1);
}

TEST(parser, comment) {
    auto block = parse("a = 1\n  # foo\n");
    ASSERT_EQ(block->statements.size(), 1);
}

TEST(parser, comment2) {
    auto block = parse("a = 1\n  # foo\nb = 2\n");
    ASSERT_EQ(block->statements.size(), 2);
}

TEST(parser, comment_in_if) {
    auto block = parse("if true\n  # comment\n  a = 2\nendif");
    ASSERT_EQ(block->statements.size(), 1);
}

TEST(parser, inline_comment) {
    auto block = parse("a = b  # foo\n");
    ASSERT_EQ(block->statements.size(), 1);
}

TEST(parser, inline_comment2) {
    auto block = parse("a = b  # foo\nb = 2");
    ASSERT_EQ(block->statements.size(), 2);
}

TEST(parser, multiple_newlines) {
    auto block = parse("a = b\n\n\nb = 2");
    ASSERT_EQ(block->statements.size(), 2);
}

TEST(parser, empty) {
    auto block = parse("# This file has no statmements\n  # or exepressions.");
    ASSERT_EQ(block->statements.size(), 0);
}
