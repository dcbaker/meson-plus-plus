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
    ASSERT_EQ(irlist.size(), 1);
    auto obj = std::move(irlist.front());
    irlist.pop_front(); // Just good house keeping
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Number>>(obj));
    auto ir = std::move(std::get<std::unique_ptr<HIR::Number>>(obj));
    ASSERT_EQ(ir->value, 7);
}

TEST(ast_to_ir, boolean) {
    auto irlist = lower("true");
    ASSERT_EQ(irlist.size(), 1);
    auto obj = std::move(irlist.front());
    irlist.pop_front(); // Just good house keeping
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::Boolean>>(obj));
    auto ir = std::move(std::get<std::unique_ptr<HIR::Boolean>>(obj));
    ASSERT_EQ(ir->value, true);
}

TEST(ast_to_ir, string) {
    auto irlist = lower("'true'");
    ASSERT_EQ(irlist.size(), 1);
    auto obj = std::move(irlist.front());
    irlist.pop_front(); // Just good house keeping
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<HIR::String>>(obj));
    auto ir = std::move(std::get<std::unique_ptr<HIR::String>>(obj));
    ASSERT_EQ(ir->value, "true");
}
