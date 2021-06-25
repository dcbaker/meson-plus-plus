// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>
#include <sstream>
#include <variant>

#include "ast_to_mir.hpp"
#include "driver.hpp"
#include "mir.hpp"
#include "passes.hpp"

namespace {

std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in) {
    Frontend::Driver drv{};
    std::istringstream stream{in};
    drv.name = "test file name";
    auto block = drv.parse(stream);
    return block;
}

MIR::IRList lower(const std::string & in) {
    auto block = parse(in);
    auto ir = MIR::lower_ast(block);
    return ir;
}

} // namespace

TEST(branch_pruning, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nendif\n");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_EQ(irlist.instructions.size(), 2);
}

TEST(branch_pruning, if_else) {
    auto irlist = lower("x = 7\nif true\n x = 8\nelse\n x = 9\nendif\n");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_EQ(irlist.instructions.size(), 2);
}

TEST(branch_pruning, if_false) {
    auto irlist = lower("x = 7\nif false\n x = 8\nelse\n x = 9\n y = 2\nendif\n");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    // Using 3 here allows us to know that we went down the right path
    ASSERT_EQ(irlist.instructions.size(), 3);

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.front());
    ASSERT_EQ(first->value, 7);
    ASSERT_EQ(first->var.name, "x");

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.back());
    ASSERT_EQ(last->value, 2);
    ASSERT_EQ(last->var.name, "y");
}
