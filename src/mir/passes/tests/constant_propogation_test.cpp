// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"

#include "test_utils.hpp"

TEST(constant_propogation, phi_should_not_propogate) {
    auto irlist = lower(R"EOF(
        if some_var
            x = 9
        else
            x = 10
        endif
        message(x)
        )EOF");
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ValueTable vt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, vt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, vt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, lst); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::constant_folding(b, rt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::constant_propogation(b, pt); },
                });

    const auto & fin = get_bb(get_con(irlist.next)->if_true->next);

    ASSERT_EQ(fin->instructions.size(), 2);

    const auto & phi_obj = fin->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Phi>(*phi_obj.obj_ptr));
    ASSERT_EQ(phi_obj.var.name, "x");
    ASSERT_EQ(phi_obj.var.version, 3);

    const auto & func_obj = fin->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*func_obj.obj_ptr));
    const auto & func = std::get<MIR::FunctionCall>(*func_obj.obj_ptr);

    const auto & arg_obj = func.pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*arg_obj.obj_ptr));
    const auto & id = std::get<MIR::Identifier>(*arg_obj.obj_ptr);
    ASSERT_EQ(id.value, "x");
    ASSERT_EQ(id.version, 3);
}

TEST(constant_propogation, function_arguments) {
    auto irlist = lower(R"EOF(
        if true
            x = 'true'
        else
            x = 'false'
        endif
        message(x)
        )EOF");
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ValueTable vt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, vt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, vt); },
                    MIR::Passes::branch_pruning,
                    MIR::Passes::join_blocks,
                    MIR::Passes::fixup_phis,
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, lst); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::constant_folding(b, rt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::constant_propogation(b, pt); },
                });

    ASSERT_EQ(irlist.instructions.size(), 2);

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*func_obj.obj_ptr));
    const auto & func = std::get<MIR::FunctionCall>(*func_obj.obj_ptr);

    const auto & arg_obj = func.pos_args.front();
    ASSERT_TRUE(std::holds_alternative<MIR::String>(*arg_obj.obj_ptr));
    const auto & str = std::get<MIR::String>(*arg_obj.obj_ptr);
    ASSERT_EQ(str.value, "true");
}

TEST(constant_propogation, array) {
    auto irlist = lower(R"EOF(
        if true
            x = 'true'
        else
            x = 'false'
        endif
        y = [x]
        )EOF");
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ValueTable vt{};

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, vt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::insert_phis(b, vt); },
                    MIR::Passes::branch_pruning,
                    MIR::Passes::join_blocks,
                    MIR::Passes::fixup_phis,
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, lst); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::constant_folding(b, rt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::constant_propogation(b, pt); },
                });

    ASSERT_EQ(irlist.instructions.size(), 2);

    const auto & func_obj = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::Array>(*func_obj.obj_ptr));
    const auto & func = std::get<MIR::Array>(*func_obj.obj_ptr);

    const auto & arg_obj = func.value.front();
    ASSERT_TRUE(std::holds_alternative<MIR::String>(*arg_obj.obj_ptr));
    const auto & str = std::get<MIR::String>(*arg_obj.obj_ptr);
    ASSERT_EQ(str.value, "true");
}

TEST(constant_propogation, method_holder) {
    auto irlist = lower(R"EOF(
        x = find_program('sh')
        x.found()
        )EOF");
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ValueTable vt{};
    MIR::State::Persistant pstate{"foo", "bar"};

    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::threaded_lowering(b, pstate); },
                });
    bool progress = MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, vt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, lst); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::constant_folding(b, rt); },
                    [&](MIR::BasicBlock & b) { return MIR::Passes::constant_propogation(b, pt); },
                });

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 2);

    const auto & front = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Program>(*front.obj_ptr));

    const auto & back = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*back.obj_ptr));

    const auto & f = std::get<MIR::FunctionCall>(*back.obj_ptr);
    ASSERT_TRUE(std::holds_alternative<MIR::Program>(*f.holder.obj_ptr));
}

TEST(constant_propogation, into_function_call) {
    auto irlist = lower(R"EOF(
        x = find_program('sh', required : false)
        assert(x.found())
        )EOF");
    MIR::Passes::LastSeenTable lst{};
    MIR::Passes::ReplacementTable rt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ValueTable vt{};
    MIR::State::Persistant pstate{"foo", "bar"};

    MIR::Passes::block_walker(
        irlist, {
                    [&](MIR::BasicBlock & b) { return MIR::Passes::threaded_lowering(b, pstate); },
                });
    bool progress = MIR::Passes::block_walker(
        irlist,
        {
            [&](MIR::BasicBlock & b) { return MIR::Passes::value_numbering(b, vt); },
            [&](MIR::BasicBlock & b) { return MIR::Passes::usage_numbering(b, lst); },
            [&](MIR::BasicBlock & b) { return MIR::Passes::constant_folding(b, rt); },
            [&](MIR::BasicBlock & b) { return MIR::Passes::constant_propogation(b, pt); },
            [&](MIR::BasicBlock & b) { return MIR::Passes::lower_program_objects(b, pstate); },
        });

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 2);

    const auto & front = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Program>(*front.obj_ptr));

    const auto & back = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*back.obj_ptr));

    const auto & f = std::get<MIR::FunctionCall>(*back.obj_ptr);
    ASSERT_TRUE(std::holds_alternative<MIR::Boolean>(*f.pos_args[0].obj_ptr));
}
