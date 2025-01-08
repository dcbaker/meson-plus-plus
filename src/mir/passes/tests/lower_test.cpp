// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(lower, trivial) {
    auto irlist = lower("project('foo')");
    MIR::State::Persistant pstate = make_pstate();
    MIR::Passes::lower_project(irlist, pstate);
    MIR::lower(irlist, pstate);
}

TEST(lower, after_files) {
    auto irlist = lower(R"EOF(
        custom_target(
            'gen',
            output : 'gen.c',
            command : ['cp', files('foo'), '@OUTPUT@']
        )
        )EOF");
    MIR::State::Persistant pstate = make_pstate();
    MIR::lower(irlist, pstate);

    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & ct_obj = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::CustomTargetPtr>(ct_obj));
}

#if false
TEST(lower, simple_real) {
    auto irlist = lower(R"EOF(
        project('foo', 'c')

        t_files = files(
            'bar.c',
        )

        executable(
            'exe',
            t_files,
        )
    )EOF");
    MIR::State::Persistant pstate = make_pstate();
    MIR::Passes::lower_project(irlist, pstate);
    MIR::lower(irlist, pstate);
}
#endif
