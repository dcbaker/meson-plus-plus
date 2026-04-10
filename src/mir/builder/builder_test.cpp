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

TEST(MIR_Builder, build_inst) {
    Builder b{};
    auto s = b.new_inst<String>("bar");
    auto i = s.finalize();
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
    auto n = b.add_inst(b.new_inst<String>("bar").set_var("x").finalize()).finalize();
    auto && insts = n->block->instructions;
    ASSERT_EQ(insts.size(), 1);

    auto && f = insts.front();
    ASSERT_EQ(f->variable.m_name, "x");
}
