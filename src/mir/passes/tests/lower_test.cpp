// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(lower, trivial) {
    auto irlist = lower("project('foo')");
    MIR::State::Persistant pstate{src_root, build_root};
    MIR::Passes::lower_project(irlist, pstate);
    MIR::lower(irlist, pstate);
}

TEST(lower, after_files) {
    auto irlist = lower(R"EOF(
        custom_target(
            'gen',
            output : 'gen.c',
            command : ['prog', files('foo'), '@OUTPUT@']
        )
        )EOF");
    MIR::State::Persistant pstate{src_root, build_root};
    // MIR::Passes::lower_project(irlist, pstate);
    MIR::lower(irlist, pstate);

    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & ct_obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::CustomTarget>>(ct_obj));
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
    MIR::State::Persistant pstate{src_root, build_root};
    MIR::Passes::lower_project(irlist, pstate);
    MIR::lower(irlist, pstate);
}
#endif
