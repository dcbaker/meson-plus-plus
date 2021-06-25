// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>
#include <sstream>
#include <variant>

#include "ast_to_hir.hpp"
#include "driver.hpp"
#include "hir.hpp"
#include "passes.hpp"

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

TEST(branch_pruning, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nendif\n");
    bool progress = HIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_EQ(irlist.instructions.size(), 2);
}

TEST(branch_pruning, if_else) {
    auto irlist = lower("x = 7\nif true\n x = 8\nelse\n x = 9\nendif\n");
    bool progress = HIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_EQ(irlist.instructions.size(), 2);
}
