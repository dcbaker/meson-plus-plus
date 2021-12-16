// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"

#include "test_utils.hpp"

TEST(value_numbering, simple) {
    auto irlist = lower(R"EOF(
        x = 7
        x = 8
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::value_numbering(&irlist, data);

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.front());
    ASSERT_EQ(first->var.version, 1);

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.back());
    ASSERT_EQ(last->var.version, 2);
}

TEST(value_numbering, branching) {
    auto irlist = lower(R"EOF(
        x = 7
        x = 8
        if true
            x = 9
        else
            x = 10
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::block_walker(
        &irlist, {[&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); }});

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.front());
    ASSERT_EQ(first->var.version, 1);

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.back());
    ASSERT_EQ(last->var.version, 2);

    const auto & bb1 = get_con(irlist.next)->if_false;
    const auto & bb1_val = std::get<std::unique_ptr<MIR::Number>>(bb1->instructions.front());
    ASSERT_EQ(bb1_val->var.version, 3);

    const auto & bb2 = get_con(irlist.next)->if_true;
    const auto & bb2_val = std::get<std::unique_ptr<MIR::Number>>(bb2->instructions.front());
    ASSERT_EQ(bb2_val->var.version, 4);
}

TEST(value_numbering, three_branch) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        elif y
            x = 10
        else
            x = 11
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::block_walker(
        &irlist, {[&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); }});

    const auto & bb1 = get_con(irlist.next)->if_true;
    const auto & bb1_val = std::get<std::unique_ptr<MIR::Number>>(bb1->instructions.front());
    ASSERT_EQ(bb1_val->var.version, 1);

    const auto & con2 = get_con(get_con(irlist.next)->if_false->next);

    const auto & bb2 = con2->if_false;
    const auto & bb2_val = std::get<std::unique_ptr<MIR::Number>>(bb2->instructions.front());
    ASSERT_EQ(bb2_val->var.version, 2);

    const auto & bb3 = con2->if_true;
    const auto & bb3_val = std::get<std::unique_ptr<MIR::Number>>(bb3->instructions.front());
    ASSERT_EQ(bb3_val->var.version, 3);
}
