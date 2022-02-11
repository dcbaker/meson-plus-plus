// SPDX-License-Identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"

#include "test_utils.hpp"

TEST(unreachable_code, clear_dead_instructions) {
    auto irlist = lower(R"EOF(
        message('bar')
        error('should be dead')
        warning('should be deleted')
        )EOF");

    MIR::State::Persistant pstate{"", ""};

    MIR::Passes::block_walker(irlist, {
                                          [&](MIR::BasicBlock & b) {
                                              return MIR::Passes::lower_free_functions(b, pstate);
                                          },
                                          MIR::Passes::delete_unreachable,
                                      });

    ASSERT_EQ(irlist.instructions.size(), 2);

    const auto & msg_obj = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Message>>(msg_obj));
    const auto & msg = *std::get<std::unique_ptr<MIR::Message>>(msg_obj);
    ASSERT_EQ(msg.level, MIR::MessageLevel::MESSAGE);

    const auto & err_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Message>>(err_obj));
    const auto & err = *std::get<std::unique_ptr<MIR::Message>>(err_obj);
    ASSERT_EQ(err.level, MIR::MessageLevel::ERROR);
}

TEST(unreachable_code, clear_next) {
    auto irlist = lower(R"EOF(
        if some_var
            error('foo')
        else
            x = 10
        endif
        message(x)
        )EOF");

    MIR::State::Persistant pstate{"", ""};

    MIR::Passes::block_walker(irlist, {
                                          [&](MIR::BasicBlock & b) {
                                              return MIR::Passes::lower_free_functions(b, pstate);
                                          },
                                          MIR::Passes::delete_unreachable,
                                      });

    const auto & branch = *get_bb(get_con(irlist.next)->if_true);
    ASSERT_EQ(branch.instructions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::monostate>(branch.next));

    const auto & fin = *get_bb(get_bb(get_con(irlist.next)->if_false)->next);
    ASSERT_EQ(fin.parents.size(), 1);
}
