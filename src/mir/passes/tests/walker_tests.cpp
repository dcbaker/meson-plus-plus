// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "passes.hpp"
#include "passes/private.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>

#include <unordered_map>

TEST(block_walker, simple) {
    std::unordered_map<uint32_t, uint32_t> seen{};

    auto && tester = [&seen](MIR::BasicBlock & b) -> bool {
        seen[b.index]++;
        return false;
    };

    auto irlist = lower(R"EOF(
        if true
            x = 7
        else
            x = 8
        endif
        )EOF");

    MIR::Passes::block_walker(irlist, {tester});
    for (auto && [block_id, count] : seen) {
        EXPECT_EQ(count, 1) << "block " << block_id << " visited " << count
                            << " times instead of 1";
    }
}

TEST(block_walker, parents_first) {
    std::vector<uint32_t> seen;

    auto && tester = [&seen](MIR::BasicBlock & b) -> bool {
        seen.emplace_back(b.index);
        return false;
    };

    auto irlist = lower(R"EOF(
        a = 0
        if true
            a = 1
        else
            a = 2
        endif
        a = 3
        )EOF");

    MIR::Passes::block_walker(irlist, {tester});
    ASSERT_EQ(seen.size(), 4);
    EXPECT_EQ(seen[0], irlist.index);

    auto & con = *std::get<std::unique_ptr<MIR::Condition>>(irlist.next);
    auto i = con.if_false.get()->index;
    EXPECT_TRUE(seen[1] == i || seen[2] == i);
    i = con.if_true.get()->index;
    EXPECT_TRUE(seen[1] == i || seen[2] == i);

    auto & last = *std::get<std::shared_ptr<MIR::BasicBlock>>(con.if_true.get()->next);
    ASSERT_EQ(seen[3], last.index);
}
