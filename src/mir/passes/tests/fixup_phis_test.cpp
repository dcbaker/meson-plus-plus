// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"

#include "test_utils.hpp"

TEST(fixup_phi, simple) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        else
            x = 10
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                });
    MIR::Passes::block_walker(irlist, {
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                      });

    ASSERT_EQ(irlist.instructions.size(), 2);

    {
        const auto & id_obj = irlist.instructions.front();
        ASSERT_EQ(id_obj.var.name, "x");
        ASSERT_EQ(id_obj.var.version, 2);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Number>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, 9);
    }

    {
        const auto & id_obj = irlist.instructions.back();
        ASSERT_EQ(id_obj.var.name, "x");
        ASSERT_EQ(id_obj.var.version, 3);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.version, 2);
    }
}

TEST(fixup_phi, three_branches) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        elif y
            x = 11
        else
            x = 10
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                });
    MIR::Passes::block_walker(irlist, {
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                      });

    ASSERT_EQ(irlist.instructions.size(), 3);

    auto it = irlist.instructions.begin();

    {
        const auto & id_obj = *it;
        ASSERT_EQ(id_obj.var.name, "x");
        ASSERT_EQ(id_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Number>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, 9);
    }

    {
        const auto & id_obj = *(++it);
        ASSERT_EQ(id_obj.var.version, 4);
        ASSERT_EQ(id_obj.var.name, "x");

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.version, 1);
    }

    {
        const auto & id_obj = *(++it);
        ASSERT_EQ(id_obj.var.name, "x");
        ASSERT_EQ(id_obj.var.version, 5);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.version, 4);
    }
}

TEST(fixup_phi, nested_branches) {
    auto irlist = lower(R"EOF(
        x = 9
        if true
            if true
                x = 11
            else
                x = 10
            endif
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};

    bool progress = true;
    while (progress) {
        progress = MIR::Passes::block_walker(
            irlist, {
                        [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, data); },
                        [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, data); },
                        MIR::Passes::branch_pruning,
                        MIR::Passes::join_blocks,
                        MIR::Passes::fixup_phis,
                    });
    }

    ASSERT_TRUE(std::holds_alternative<std::monostate>(irlist.next));
    ASSERT_EQ(irlist.instructions.size(), 4);
    auto it = irlist.instructions.begin();

    {
        const auto & id_obj = *it;
        ASSERT_EQ(id_obj.var.name, "x");
        ASSERT_EQ(id_obj.var.version, 1);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Number>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, 9);
    }

    ++it;

    {
        const auto & id_obj = *it;
        ASSERT_EQ(id_obj.var.name, "x");
        ASSERT_EQ(id_obj.var.version, 3);

        ASSERT_TRUE(std::holds_alternative<MIR::Number>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Number>(*id_obj.obj_ptr);
        ASSERT_EQ(id.value, 11);
    }

    ++it;

    {
        const auto & id_obj = *it;
        ASSERT_EQ(id_obj.var.name, "x");
        ASSERT_EQ(id_obj.var.version, 4);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.version, 3);
    }

    ++it;

    {
        const auto & id_obj = *it;
        ASSERT_EQ(id_obj.var.name, "x");
        ASSERT_EQ(id_obj.var.version, 5);

        ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*id_obj.obj_ptr));
        const auto & id = std::get<MIR::Identifier>(*id_obj.obj_ptr);
        ASSERT_EQ(id.version, 4);
    }
}
