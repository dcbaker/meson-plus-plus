// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "ast_to_mir.hpp"
#include "driver.hpp"
#include "ir.hpp"
#include "node.hpp"
#include "test_helpers.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <variant>

namespace {

/// @brief Convert Meson DSL into
/// @param in
/// @return
static MIR::IR::CFG parse(const std::string & in) {
    Frontend::Driver drv{};
    std::istringstream stream{in};
    drv.name = "test file name";
    auto block = drv.parse(stream);
    auto cfg = MIR::ast_to_mir(block);

    return cfg;
}

} // namespace

using namespace MIR::UT;

TEST(ast_to_mir, simple) {
    MIR::IR::CFG cfg{parse("'foo'")};
    ASSERT_EQ(cfg.head()->instructions.size(), 1);
    ASSERT_TRUE(holds<MIR::IR::String>(get_ir(cfg, -1).instruction));
}

TEST(ast_to_mir, assignment) {
    MIR::IR::CFG cfg{parse("x = 'foo'")};
    EXPECT_EQ(cfg.head()->instructions.size(), 1);

    const MIR::IR::Instruction & ir = get_ir(cfg, 0);
    EXPECT_TRUE(holds<MIR::IR::String>(ir.instruction));
    ASSERT_EQ(ir.variable.m_name, "x");
}

TEST(ast_to_mir, only_if) {
    /*
     * We should have three blocks, with a diamond-like configuration
     *
     *              O block before if
     *             / \
     *    if body O   |
     *             \ /
     *              O tail
     *
     * The first block should have a condition of true
     *
     * The `if` body should have one instruction, the assignment
     *
     * The `tail` block should have no instructions
     */
    const std::string code{R"EOF(
        if true
            x = 0
        endif
        )EOF"};
    MIR::IR::CFG cfg = parse(code);

    const MIR::IR::BasicBlock & root = *cfg.head();
    ASSERT_EQ(root.instructions.size(), 1);
    EXPECT_TRUE(holds<MIR::IR::Boolean>(get_ir(root, 0).instruction));

    const MIR::IR::BasicBlock & body = *cfg.head()->left_successor();
    EXPECT_EQ(body.instructions.size(), 1);

    auto && ir = get_ir(body, 0);
    EXPECT_EQ(ir.variable.m_name, "x");
    EXPECT_TRUE(holds<MIR::IR::Number>(ir.instruction));
    EXPECT_EQ(get<MIR::IR::Number>(ir.instruction).value, 0);

    const MIR::IR::BasicBlock & tail = *cfg.head()->right_successor();
    EXPECT_EQ(tail.instructions.size(), 0);

    EXPECT_EQ(*body.left_successor(), tail);
}

TEST(ast_to_mir, if_else) {
    const std::string code{R"EOF(
        if true
            x = 0
        else
            x = 1
        endif
        )EOF"};
    MIR::IR::CFG cfg = parse(code);

    /*
     * We should have three blocks, with a diamond-like configuration
     *
     *              O block before if
     *             / \
     *    if body O   O else body
     *             \ /
     *              O tail
     *
     * The first block should have a condition of true
     *
     * The `if` body should have one instruction, the assignment
     *
     * The `tail` block should have no instructions
     */

    const MIR::IR::BasicBlock & root = *cfg.head();
    ASSERT_EQ(root.instructions.size(), 1);
    EXPECT_TRUE(holds<MIR::IR::Boolean>(get_ir(root, -1).instruction));

    const MIR::IR::BasicBlock & body = *cfg.head()->left_successor();
    EXPECT_EQ(body.instructions.size(), 1);

    const MIR::IR::BasicBlock & el = *cfg.head()->right_successor();
    EXPECT_EQ(el.instructions.size(), 1);

    EXPECT_EQ(body.left_successor(), el.left_successor());
    EXPECT_EQ(body.left_successor()->instructions.size(), 0);
}

// TODO: test for if/elif
// TODO: test for if/elif/else
// TODO: test nested blocks

// TODO: test for foreach

TEST(ast_to_mir, foreach_array_simple) {
    /*
     * We should end up with a structure like:
     *
     *              O preamble
     *              |
     *       header O<--|
     *             / \ /
     *            |   O body
     *             \
     *              O tail
     *
     */

    const std::string code{R"EOF(
        foreach x : ['a', 'b', 'c']
            message(x)
        endforeach
        )EOF"};
    MIR::IR::CFG cfg = parse(code);

    const MIR::IR::BasicBlock & preamble = *cfg.head()->left_successor();
    ASSERT_EQ(preamble.instructions.size(), 4);
    EXPECT_TRUE(holds<MIR::IR::Undefined>(get_ir(preamble, 0).instruction));
    EXPECT_TRUE(holds<MIR::IR::Array>(get_ir(preamble, 1).instruction));
    EXPECT_TRUE(holds<MIR::IR::FunctionCall>(get_ir(preamble, 2).instruction));
    EXPECT_TRUE(holds<MIR::IR::Number>(get_ir(preamble, 3).instruction));

    const MIR::IR::BasicBlock & header = *preamble.left_successor();
    ASSERT_EQ(header.instructions.size(), 4);
    EXPECT_TRUE(holds<MIR::IR::FunctionCall>(get_ir(header, 0).instruction));
    EXPECT_TRUE(holds<MIR::IR::FunctionCall>(get_ir(header, 1).instruction));
    EXPECT_TRUE(holds<MIR::IR::FunctionCall>(get_ir(header, 2).instruction));
    EXPECT_TRUE(holds<MIR::IR::Identifier>(get_ir(header, 3).instruction));

    const MIR::IR::BasicBlock & tail = *header.right_successor();
    EXPECT_EQ(tail.instructions.size(), 0);

    const MIR::IR::BasicBlock & body = *header.left_successor();
    ASSERT_EQ(body.instructions.size(), 1);
    EXPECT_TRUE(holds<MIR::IR::FunctionCall>(get_ir(body, 0).instruction));

    ASSERT_EQ(*body.left_successor(), header);
}
