// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"

#include "test_utils.hpp"

TEST(constant_propogation, phi_should_not_propogate) {
    auto irlist = lower(R"EOF(
        if some_func()
            x = 9
        else
            x = 10
        endif
        message(x)
        )EOF");

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}});
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::ConstantFolding{},
                                          MIR::Passes::ConstantPropagation{},
                                      });

    const auto & branches = std::get<MIR::Branch>(*irlist->block->instructions.front().obj_ptr);
    const auto & arm = std::get<1>(branches.branches.at(0));
    const auto & tail = std::get<MIR::Jump>(*arm->block->instructions.back().obj_ptr).target;
    const auto & msg = std::get<MIR::FunctionCall>(*tail->block->instructions.back().obj_ptr);
    ASSERT_TRUE(std::holds_alternative<MIR::Identifier>(*msg.pos_args.at(0).obj_ptr));
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

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                          MIR::Passes::ConstantFolding{},
                                          MIR::Passes::ConstantPropagation{},
                                      });

    ASSERT_EQ(irlist->block->instructions.size(), 2);

    const auto & func_obj = irlist->block->instructions.back();
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

    // We do this in two walks because we don't have all of passes necissary to
    // get the state we want to test.
    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          MIR::Passes::branch_pruning,
                                          MIR::Passes::join_blocks,
                                          MIR::Passes::fixup_phis,
                                          MIR::Passes::ConstantFolding{},
                                          MIR::Passes::ConstantPropagation{},
                                      });

    ASSERT_EQ(irlist->block->instructions.size(), 2);

    const auto & func_obj = irlist->block->instructions.back();
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
    MIR::State::Persistant pstate = make_pstate();
    MIR::Passes::Printer printer{};

    bool progress =
        MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}, std::ref(printer)});
    ASSERT_TRUE(progress) << "GVN did not make progress";

    printer.increment();
    progress =
        MIR::Passes::graph_walker(irlist, {
                                              [&](std::shared_ptr<MIR::CFGNode> b) {
                                                  return MIR::Passes::threaded_lowering(b, pstate);
                                              },
                                              std::ref(printer),
                                          });
    ASSERT_TRUE(progress) << "threaded lowering did not make progress";

    printer.increment();
    progress = MIR::Passes::graph_walker(irlist, {
                                                     MIR::Passes::ConstantFolding{},
                                                     MIR::Passes::ConstantPropagation{},
                                                     std::ref(printer),
                                                 });
    ASSERT_TRUE(progress) << "constant folding/propagation did not make progress";
    ASSERT_EQ(irlist->block->instructions.size(), 2);

    const auto & front = irlist->block->instructions.front();
    EXPECT_TRUE(std::holds_alternative<MIR::Program>(*front.obj_ptr));

    const auto & back = irlist->block->instructions.back();
    EXPECT_TRUE(std::holds_alternative<MIR::FunctionCall>(*back.obj_ptr));

    const auto & f = std::get<MIR::FunctionCall>(*back.obj_ptr);
    ASSERT_TRUE(std::holds_alternative<MIR::Program>(*f.holder.obj_ptr));
}

TEST(constant_propogation, into_function_call) {
    auto irlist = lower(R"EOF(
        x = find_program('sh', required : false)
        assert(x.found())
        )EOF");
    MIR::State::Persistant pstate = make_pstate();

    MIR::Passes::Printer printer{};

    MIR::Passes::graph_walker(irlist, {MIR::Passes::GlobalValueNumbering{}, std::ref(printer)});
    printer.increment();
    MIR::Passes::graph_walker(irlist, {
                                          [&](std::shared_ptr<MIR::CFGNode> b) {
                                              return MIR::Passes::threaded_lowering(b, pstate);
                                          },
                                          std::ref(printer),
                                      });
    printer.increment();
    bool progress = MIR::Passes::graph_walker(
        irlist,
        {
            MIR::Passes::ConstantFolding{},
            MIR::Passes::ConstantPropagation{},
            [&](std::shared_ptr<MIR::CFGNode> b) {
                return MIR::Passes::instruction_walker(*b, {[&pstate](const MIR::Instruction & i) {
                    return MIR::Passes::lower_program_objects(i, pstate);
                }});
            },
            std::ref(printer),
        });

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 2);

    const auto & front = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Program>(*front.obj_ptr));

    const auto & back = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*back.obj_ptr));

    const auto & f = std::get<MIR::FunctionCall>(*back.obj_ptr);
    ASSERT_TRUE(std::holds_alternative<MIR::Boolean>(*f.pos_args[0].obj_ptr));
}
