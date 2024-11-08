// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"
#include "toolchains/archiver.hpp"
#include "toolchains/common.hpp"
#include "toolchains/compilers/cpp/cpp.hpp"
#include "toolchains/linker.hpp"

#include "test_utils.hpp"

TEST(machine_lower, simple) {
    auto irlist = lower("x = 7\ny = host_machine.cpu_family()");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 2);
    const auto & r = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::String>(*r.obj_ptr));
    ASSERT_EQ(std::get<MIR::String>(*r.obj_ptr).value, "x86_64");
}

TEST(machine_lower, in_array) {
    auto irlist = lower("x = [host_machine.cpu_family()]");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & r = irlist->block->instructions.front();

    ASSERT_TRUE(std::holds_alternative<MIR::Array>(*r.obj_ptr));
    const auto & a = std::get<MIR::Array>(*r.obj_ptr).value;

    ASSERT_EQ(a.size(), 1);
    ASSERT_TRUE(std::holds_alternative<MIR::String>(*a[0].obj_ptr));
    ASSERT_EQ(std::get<MIR::String>(*a[0].obj_ptr).value, "x86_64");
}

TEST(machine_lower, in_function_args) {
    auto irlist = lower("foo(host_machine.endian())");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & r = irlist->block->instructions.front();

    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCall>(*r.obj_ptr));
    const auto & f = std::get<MIR::FunctionCall>(*r.obj_ptr);

    ASSERT_EQ(f.pos_args.size(), 1);
    ASSERT_TRUE(std::holds_alternative<MIR::String>(*f.pos_args[0].obj_ptr));
    ASSERT_EQ(std::get<MIR::String>(*f.pos_args[0].obj_ptr).value, "little");
}

TEST(machine_lower, in_condition) {
    auto irlist = lower("if host_machine.cpu_family()\n x = 2\nendif");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & obj = std::get<0>(
        std::get<MIR::Branch>(*irlist->block->instructions.back().obj_ptr).branches.at(0));
    ASSERT_TRUE(std::holds_alternative<MIR::String>(*obj.obj_ptr));
    ASSERT_EQ(std::get<MIR::String>(*obj.obj_ptr).value, "x86_64");
}
