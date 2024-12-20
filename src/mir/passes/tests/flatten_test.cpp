// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"

#include "test_utils.hpp"

namespace {

bool wrapper(std::shared_ptr<MIR::CFGNode> node) {
    return MIR::Passes::instruction_walker(*node, {MIR::Passes::flatten});
}

} // namespace

TEST(flatten, basic) {
    auto irlist = lower("func(['a', ['b', ['c']], 'd'])");
    bool progress = wrapper(irlist);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();

    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*r.obj_ptr));
    const auto & f = std::get<MIR::FunctionCall>(*r.obj_ptr);
    ASSERT_EQ(f.name, "func");
    ASSERT_EQ(f.pos_args.size(), 1);

    const auto & arg = f.pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Array>(*arg.obj_ptr));
    const auto & arr = std::get<MIR::Array>(*arg.obj_ptr).value;

    ASSERT_EQ(arr.size(), 4);
}

TEST(flatten, already_flat) {
    auto irlist = lower("func(['a', 'd'])");
    bool progress = wrapper(irlist);

    ASSERT_FALSE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();

    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*r.obj_ptr));
    const auto & f = std::get<MIR::FunctionCall>(*r.obj_ptr);
    ASSERT_EQ(f.name, "func");
    ASSERT_EQ(f.pos_args.size(), 1);

    const auto & arg = f.pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Array>(*arg.obj_ptr));
    const auto & arr = std::get<MIR::Array>(*arg.obj_ptr).value;

    ASSERT_EQ(arr.size(), 2);
}

TEST(flatten, mixed_args) {
    auto irlist = lower("project('foo', ['a', ['d']])");
    bool progress = wrapper(irlist);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();

    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*r.obj_ptr));
    const auto & f = std::get<MIR::FunctionCall>(*r.obj_ptr);
    ASSERT_EQ(f.pos_args.size(), 2);

    const auto & arg = f.pos_args.back();
    ASSERT_TRUE(std::holds_alternative<MIR::Array>(*arg.obj_ptr));
    const auto & arr = std::get<MIR::Array>(*arg.obj_ptr).value;

    ASSERT_EQ(arr.size(), 2);
}

TEST(flatten, keyword_mixed) {
    auto irlist = lower("func(arg : ['foo', ['bar', ['foobar']]])");

    bool progress = wrapper(irlist);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();

    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*r.obj_ptr));
    const auto & f = std::get<MIR::FunctionCall>(*r.obj_ptr);
    const auto & arr = std::get<MIR::Array>(*f.kw_args.at("arg").obj_ptr);
    ASSERT_EQ(arr.value.size(), 3);
}
