// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "node.hpp"
#include "scanner.hpp"
#include "parser.yy.hpp"

static std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in) {
    std::istringstream stream {in};
    auto block = std::make_unique<Frontend::AST::CodeBlock>();
    auto scanner = std::make_unique<Frontend::Scanner>(&stream);
    auto parser = std::make_unique<Frontend::Parser>(*scanner, block);
    parser->parse();
    return block;
}

TEST(parser, string) {
    auto block = parse("'foo'");
    ASSERT_EQ(block->as_string(), "'foo'");
}

TEST(parser, decminal_number) {
    auto block = parse("77");
    ASSERT_EQ(block->as_string(), "77");
}

TEST(parser, octal_number) {
    auto block = parse("0o10");
    ASSERT_EQ(block->as_string(), "8");
}

TEST(parser, hex_number) {
    auto block = parse("0xf");
    ASSERT_EQ(block->as_string(), "15");
}

TEST(parser, identifier) {
    auto block = parse("foo");
    ASSERT_EQ(block->as_string(), "foo");
}

TEST(parser, multiplication) {
    auto block = parse("5  * 4 ");
    ASSERT_EQ(block->as_string(), "5 * 4");
}

TEST(parser, division) {
    auto block = parse("5 / 4 ");
    ASSERT_EQ(block->as_string(), "5 / 4");
}

TEST(parser, addition) {
    auto block = parse("5 + 4 ");
    ASSERT_EQ(block->as_string(), "5 + 4");
}

TEST(parser, subtraction) {
    auto block = parse("5 - 4 ");
    ASSERT_EQ(block->as_string(), "5 - 4");
}

TEST(parser, mod) {
    auto block = parse("5 % 4 ");
    ASSERT_EQ(block->as_string(), "5 % 4");
}

TEST(parser, unary_negate) {
    auto block = parse("- 5");
    ASSERT_EQ(block->as_string(), "-5");
}
