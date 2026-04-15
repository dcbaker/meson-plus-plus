// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "builder.hpp"
#include "ir.hpp"
#include "test_helpers.hpp"

#include <gtest/gtest.h>

using namespace MIR::IR;
using namespace MIR::builder;
using namespace MIR::UT;

TEST(MIR_Builder, simple) {
    Builder b{};
    auto i = std::make_unique<Instruction>(std::make_shared<String>("foo"));
    b.add_inst(std::move(i));
    auto n = b.get();
    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);
}

TEST(MIR_Builder, constructor_from_non_empty_block_no_condition) {
    Builder b{};

    // clang-format off
    b.add_inst(make_instruction<Number>(0))
     .set_cursor_begin()
     .add_inst(make_instruction<Number>(1));
    // clang-format on

    b = {b.get()};
    auto itr = b.get()->block->instructions.begin();
    EXPECT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 1);
    EXPECT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 0);

    b.add_inst(make_instruction<Number>(42));
    EXPECT_EQ(get<Number>(get_ir(b.get()->block, -1).instruction).value, 42);
}

TEST(MIR_Builder, constructor_from_non_empty_block_with_condition) {
    Builder b{};

    // clang-format off
    b.add_inst(make_instruction<Number>(0))
     .set_cursor_begin()
     .add_inst(make_instruction<Number>(1))
     .add_condition(make_instruction<Boolean>(true));
    // clang-format on

    b = {b.get()};
    auto itr = b.get()->block->instructions.begin();
    EXPECT_EQ(b.get()->block->instructions.size(), 3);
    EXPECT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 1);
    EXPECT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 0);
    EXPECT_EQ(std::get<std::shared_ptr<Boolean>>((*itr++)->instruction)->value, true);

    b.add_inst(make_instruction<Number>(42));
    EXPECT_EQ(get<Number>(get_ir(b.get()->block, -2).instruction).value, 42);
    EXPECT_EQ(get<Boolean>(get_ir(b.get()->block, -1).instruction).value, true);
}

TEST(MIR_Builder, constructor_from_empty_block) {
    Builder b{std::make_shared<Node>()};

    EXPECT_EQ(b.get()->block->instructions.size(), 0);

    b.add_inst(make_instruction<Number>(42));
    EXPECT_EQ(get<Number>(get_ir(b.get()->block, -1).instruction).value, 42);
}

TEST(MIR_Builder, make_instruction) {
    Builder b{};
    auto s = make_instruction<String>("bar");
    b.add_inst(std::move(s));
    auto n = b.get();
    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);

    auto && f = insts.front();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<String>>(f->instruction));

    auto && str = std::get<std::shared_ptr<String>>(f->instruction);
    ASSERT_EQ(str->m_value, "bar");
}

TEST(MIR_Builder, set_var) {
    Builder b{};
    auto n = b.add_inst(make_instruction<String>("bar").set_var("x")).get();
    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);

    auto && f = insts.front();
    ASSERT_EQ(f->variable.m_name, "x");
}

TEST(MIR_Builder, funccall) {
    Builder b{};
    // clang-format off
    auto n = b.add_inst(make_instruction<FunctionCall>("add")
                        .add_pos_arg(make_instruction<Number>(1))
                        .add_pos_arg(make_instruction<Number>(2))
                        .add_kw_arg(make_instruction<String>("foo"),
                                    make_instruction<Boolean>(false)))
              .get();
    // clang-format on

    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);

    // TODO: really should checkt aht this is correct...
}

TEST(MIR_Builder, cursor) {
    Builder b{};
    auto & ir = b.get()->block->instructions;

    // clang-format off
    b.add_inst(make_instruction<Number>(0))
     .add_inst(make_instruction<Number>(1));
    // clang-format on
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Number>>(ir.back()->instruction));

    b.add_condition(make_instruction<Boolean>(true));
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Boolean>>(ir.back()->instruction));

    // A new instruction should be placed before the condition
    b.add_inst(make_instruction<Number>(2));
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Boolean>>(ir.back()->instruction));
}

TEST(MIR_Builder, set_cursor_begin) {
    Builder b{};
    auto & ir = b.get()->block->instructions;

    // clang-format off
    b.add_inst(make_instruction<Number>(0))
     .add_inst(make_instruction<Number>(1))
     .set_cursor_begin()
     .add_inst(make_instruction<Number>(2));
    // clang-format on

    auto itr = ir.begin();
    ASSERT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 2);
    ASSERT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 0);
    ASSERT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 1);
}

TEST(MIR_Builder, set_cursor_end) {
    Builder b{};

    // clang-format off
    b.add_inst(make_instruction<Number>(0))
     .set_cursor_end()
     .add_inst(make_instruction<Number>(1));
    // clang-format on

    auto & ir = b.get()->block;
    EXPECT_EQ(get<Number>(get_ir(ir, 0).instruction).value, 0);
    EXPECT_EQ(get<Number>(get_ir(ir, 1).instruction).value, 1);
}

TEST(MIR_Builder, set_cursor_begin_end) {
    Builder b{};
    auto & ir = b.get()->block->instructions;

    // clang-format off
    b.add_inst(make_instruction<Number>(0))
     .set_cursor_begin()
     .add_inst(make_instruction<Number>(1))
     .set_cursor_end()
     .add_inst(make_instruction<Number>(2));
    // clang-format on

    auto itr = ir.begin();
    EXPECT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 1);
    EXPECT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 0);
    ASSERT_EQ(std::get<std::shared_ptr<Number>>((*itr++)->instruction)->value, 2);
}
