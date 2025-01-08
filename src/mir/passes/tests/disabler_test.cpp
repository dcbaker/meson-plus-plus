// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024-2025 Intel Corporation

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"

#include "test_utils.hpp"

#include <gtest/gtest.h>

namespace {

bool wrapper(std::shared_ptr<MIR::CFGNode> node) {
    const MIR::State::Persistant pstate = make_pstate();
    return MIR::Passes::instruction_walker(*node, {
                                                      [&pstate](const MIR::Object & i) {
                                                          return MIR::Passes::lower_free_functions(
                                                              i, pstate);
                                                      },
                                                      MIR::Passes::disable,
                                                  });
}

} // namespace

TEST(disabler, in_array) {
    auto irlist = lower("['foo', 'bar', 1, disabler()]");
    bool progress = wrapper(irlist);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::DisablerPtr>(r));
}

TEST(disabler, in_dict) {
    auto irlist = lower("{'a' : 'b', 'b' : 1, 'd' : disabler()}");
    bool progress = wrapper(irlist);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::DisablerPtr>(r));
}

TEST(disabler, in_function_arguments) {
    auto irlist = lower("meson.override_dependency('bar', disabler())");
    bool progress = wrapper(irlist);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::DisablerPtr>(r));
}
