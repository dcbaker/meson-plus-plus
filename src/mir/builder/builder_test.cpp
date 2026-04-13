// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "builder.hpp"
#include "ir.hpp"

#include <gtest/gtest.h>

using namespace MIR::IR;
using namespace MIR::Builder;

TEST(MIR_Builder, simple) {
    Builder b{};
    auto i = std::make_unique<Instruction>(std::make_shared<String>("foo"));
    b.add_inst(std::move(i));
    auto n = b.finalize();
    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);
}

TEST(MIR_Builder, make_instruction) {
    Builder b{};
    auto s = make_instruction<String>("bar");
    auto i = s.as_instr();
    b.add_inst(std::move(i));
    auto n = b.finalize();
    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);

    auto && f = insts.front();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<String>>(f->instruction));

    auto && str = std::get<std::shared_ptr<String>>(f->instruction);
    ASSERT_EQ(str->m_value, "bar");
}

TEST(MIR_Builder, set_var) {
    Builder b{};
    auto n = b.add_inst(make_instruction<String>("bar").set_var("x").as_instr()).finalize();
    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);

    auto && f = insts.front();
    ASSERT_EQ(f->variable.m_name, "x");
}

TEST(MIR_Builder, funccall) {
    Builder b{};
    // clang-format off
    auto n = b.add_inst(make_instruction<FunctionCall>("add")
                         .add_pos_arg(make_instruction<Number>(1).as_type())
                         .add_pos_arg(make_instruction<Number>(2).as_type())
                         .add_kw_arg(make_instruction<String>("foo").as_type(),
                                     make_instruction<Boolean>(false).as_type())
                         .as_instr())
              .finalize();
    // clang-format on

    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);

    // TODO: really should checkt aht this is correct...
}
