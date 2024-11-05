// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "passes.hpp"
#include "passes/argument_extractors.hpp"
#include "passes/private.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>

TEST(custom_target_program_replacement, string) {
    auto irlist = lower(R"EOF(
        x = custom_target('name', command : 'foo.py')
        )EOF");

    MIR::Passes::graph_walker(irlist, {MIR::Passes::custom_target_program_replacement});

    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & fc_obj = irlist->instructions.front();
    const auto & fc = std::get<MIR::FunctionCall>(*fc_obj.obj_ptr);

    const auto & commands_o =
        MIR::Passes::extract_keyword_argument<MIR::Array>(fc.kw_args, "command", "Error!");

    const auto & commands = commands_o.value();
    ASSERT_EQ(commands.value.size(), 1);

    const auto & cmd0 = std::get<MIR::FunctionCall>(*commands.value.at(0).obj_ptr);
    EXPECT_EQ(cmd0.name, "find_program");

    ASSERT_EQ(cmd0.pos_args.size(), 1);
    const auto & arg_o = MIR::Passes::extract_positional_argument<MIR::String>(cmd0.pos_args.at(0));
    ASSERT_TRUE(arg_o.has_value());
    ASSERT_EQ(arg_o.value().value, "foo.py");
}

TEST(custom_target_program_replacement, array) {
    auto irlist = lower(R"EOF(
        x = custom_target('name', command : ['foo.py', '@INPUT@', '@OUTPUT@'])
        )EOF");

    MIR::Passes::graph_walker(irlist, {MIR::Passes::custom_target_program_replacement});

    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & fc_obj = irlist->instructions.front();
    const auto & fc = std::get<MIR::FunctionCall>(*fc_obj.obj_ptr);

    const auto & commands_o =
        MIR::Passes::extract_keyword_argument<MIR::Array>(fc.kw_args, "command", "Error!");
    ASSERT_TRUE(commands_o.has_value());

    const auto & commands = commands_o.value();
    ASSERT_EQ(commands.value.size(), 3);

    const auto & cmd0 = std::get<MIR::FunctionCall>(*commands.value.at(0).obj_ptr);
    EXPECT_EQ(cmd0.name, "find_program");

    ASSERT_EQ(cmd0.pos_args.size(), 1);
    const auto & arg_o =
        MIR::Passes::extract_positional_argument<MIR::String>(cmd0.pos_args.at(0), "Error!");
    ASSERT_EQ(arg_o.value, "foo.py");
}
