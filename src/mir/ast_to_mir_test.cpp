// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "ast_to_mir.hpp"
#include "driver.hpp"
#include "ir.hpp"
#include "node.hpp"

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

/// @brief Get an instruction from the CFG by index
/// @param bb the CFG to get the instruction from
/// @param index The index of that instruction, may be either positive or negative
/// @return A const reference to that instruction
const MIR::IR::Instruction & get_ir(const MIR::IR::BasicBlock & bb, int index) {
    if (index >= 0) {
        auto itr = bb.instructions.begin();
        for (int i = 0; i < index; ++i) {
            ++itr;
        }
        return **itr;
    }
    auto itr = bb.instructions.end();
    for (int i = 0; i > index; --i) {
        --itr;
    }
    return **itr;
}

const MIR::IR::Instruction & get_ir(const MIR::IR::CFG & cfg, int index) {
    return get_ir(*cfg.root->block, index);
}

const MIR::IR::Instruction & get_ir(const MIR::IR::Node & root, int index) {
    return get_ir(*root.block, index);
}

template <typename T> bool holds(const MIR::IR::InstructionType & inst) {
    return std::holds_alternative<std::shared_ptr<T>>(inst);
}

template <typename T> const T & get(const MIR::IR::InstructionType & inst) {
    return *std::get<std::shared_ptr<T>>(inst);
}

} // namespace

TEST(ast_to_mir, simple) {
    MIR::IR::CFG cfg = parse("'foo'");
    ASSERT_EQ(cfg.root->block->instructions.size(), 1);
    ASSERT_TRUE(holds<MIR::IR::String>(get_ir(cfg, -1).instruction));
}

TEST(ast_to_mir, assignment) {
    MIR::IR::CFG cfg = parse("x = 'foo'");
    EXPECT_EQ(cfg.root->block->instructions.size(), 1);

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

    const MIR::IR::BasicBlock & root = *cfg.root->block;
    ASSERT_EQ(root.instructions.size(), 1);
    EXPECT_TRUE(holds<MIR::IR::Boolean>(get_ir(root, 0).instruction));

    const MIR::IR::Node & body = *cfg.root->successors[0];
    EXPECT_EQ(body.block->instructions.size(), 1);

    auto && ir = get_ir(body, 0);
    EXPECT_EQ(ir.variable.m_name, "x");
    EXPECT_TRUE(holds<MIR::IR::Number>(ir.instruction));
    EXPECT_EQ(get<MIR::IR::Number>(ir.instruction).value, 0);

    const MIR::IR::Node & tail = *cfg.root->successors[1];
    EXPECT_EQ(tail.block->instructions.size(), 0);

    EXPECT_EQ(*body.successors[0], tail);
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

    const MIR::IR::BasicBlock & root = *cfg.root->block;
    ASSERT_EQ(root.instructions.size(), 1);
    EXPECT_TRUE(holds<MIR::IR::Boolean>(get_ir(root, -1).instruction));

    const MIR::IR::Node & body = *cfg.root->successors[0];
    EXPECT_EQ(body.block->instructions.size(), 1);

    const MIR::IR::Node & el = *cfg.root->successors[1];
    EXPECT_EQ(el.block->instructions.size(), 1);

    EXPECT_EQ(body.successors[0], el.successors[0]);
    EXPECT_EQ(body.successors[0]->block->instructions.size(), 0);
}

// TODO: test for if/elif
// TODO: test for if/elif/else

// TODO: test for foreach
