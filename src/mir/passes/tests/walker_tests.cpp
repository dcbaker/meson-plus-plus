// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024-2025 Intel Corporation

#include "passes.hpp"
#include "passes/private.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>

#include <unordered_map>

TEST(graph_walker, simple) {
    std::unordered_map<uint32_t, uint32_t> seen{};

    auto && tester = [&seen](std::shared_ptr<MIR::CFGNode> b) -> bool {
        seen[b->index]++;
        return false;
    };

    auto irlist = lower(R"EOF(
        if true
            x = 7
        else
            x = 8
        endif
        )EOF");

    MIR::Passes::graph_walker(irlist, {tester});
    for (auto && [block_id, count] : seen) {
        EXPECT_EQ(count, 1) << "block " << block_id << " visited " << count
                            << " times instead of 1";
    }
}

TEST(graph_walker, predecessors_first) {
    std::vector<uint32_t> seen;

    auto && tester = [&seen](std::shared_ptr<MIR::CFGNode> b) -> bool {
        seen.emplace_back(b->index);
        return false;
    };

    auto node = lower(R"EOF(
        a = 0
        if true
            a = 1
        else
            a = 2
        endif
        a = 3
        )EOF");

    MIR::Passes::graph_walker(node, {tester});
    ASSERT_EQ(seen.size(), 4);
    EXPECT_EQ(seen[0], node->index);
    auto s = node->successors.begin();
    EXPECT_EQ(seen[1], (*s)->index);
    EXPECT_EQ(seen[2], (*++s)->index);
    const auto & last = std::get<MIR::JumpPtr>((*s)->block->instructions.back())->target;
    EXPECT_EQ(seen[3], last->index);
}
