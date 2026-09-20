// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "builder/builder.hpp"
#include "ir.hpp"
#include "remove_ternary.hpp"

#include <gtest/gtest.h>

using namespace MIR;
using namespace MIR::builder;

TEST(MirPasses_RemoveTernary, basic) {
    IR::CFG cfg{};

    const std::string var_name = "x";

    IR::BasicBlock * n = Builder{&cfg}
                             .add_inst(make_instruction<IR::Number>(0))
                             .add_inst(make_instruction<IR::FunctionCall>("ternary", "meson++",
                                                                          IR::FunctionId::ternary)
                                           .add_pos_arg(make_instruction<IR::Boolean>(true))
                                           .add_pos_arg(make_instruction<IR::String>("foo"))
                                           .add_pos_arg(make_instruction<IR::String>("bar"))
                                           .set_var(var_name))
                             .add_inst(make_instruction<IR::Number>(1))
                             .get();

    ASSERT_TRUE(MIR::Passes::remove_ternary(&cfg, n));
    ASSERT_EQ(n->instructions.size(), 2);

    auto lhs = n->left_successor();
    ASSERT_TRUE(lhs);
    ASSERT_EQ(lhs->instructions.size(), 1);
    ASSERT_EQ(lhs->instructions.front()->variable.m_name, var_name);

    auto rhs = n->right_successor();
    ASSERT_TRUE(rhs);
    ASSERT_EQ(rhs->instructions.size(), 1);
    ASSERT_EQ(rhs->instructions.front()->variable.m_name, var_name);

    auto tail = lhs->left_successor();
    ASSERT_TRUE(tail);
    ASSERT_EQ(tail, rhs->left_successor());
    ASSERT_EQ(tail->instructions.size(), 1);
}
