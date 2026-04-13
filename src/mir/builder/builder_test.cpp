// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "builder.hpp"
#include "ir.hpp"

#include <gtest/gtest.h>

using namespace MIR::IR;
using namespace MIR::builder;

TEST(MIR_Builder, simple) {
    Builder b{};
    auto i = std::make_unique<Instruction>(std::make_shared<String>("foo"));
    b.add_inst(std::move(i));
    auto n = b.get();
    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);
}

TEST(MIR_Builder, make_instruction) {
    Builder b{};
    auto s = make_instruction<String>("bar");
    auto i = s.as_instr();
    b.add_inst(std::move(i));
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
    auto n = b.add_inst(make_instruction<String>("bar").set_var("x").as_instr()).get();
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
                                     make_instruction<Boolean>(false))
                         .as_instr())
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
    b.add_inst(make_instruction<Number>(0).as_instr())
     .add_inst(make_instruction<Number>(1).as_instr());
    // clang-format on
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Number>>(ir.back()->instruction));

    b.add_condition(make_instruction<Boolean>(true).as_instr());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Boolean>>(ir.back()->instruction));

    // A new instruction should be placed before the condition
    b.add_inst(make_instruction<Number>(2).as_instr());
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Boolean>>(ir.back()->instruction));
}
