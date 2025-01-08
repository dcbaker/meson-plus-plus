// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"
#include "toolchains/archiver.hpp"
#include "toolchains/common.hpp"
#include "toolchains/compilers/cpp/cpp.hpp"
#include "toolchains/linker.hpp"

#include "test_utils.hpp"

namespace {

using MachineInfo = MIR::Machines::PerMachine<MIR::Machines::Info>;

bool wrapper(std::shared_ptr<MIR::CFGNode> & node, const MachineInfo & info) {
    return MIR::Passes::instruction_walker(*node, {[&info](const MIR::Object & obj) {
        return MIR::Passes::machine_lower(obj, info);
    }});
}

} // namespace

TEST(machine_lower, simple) {
    auto irlist = lower("x = 7\ny = host_machine.cpu_family()");
    auto info =
        MachineInfo(MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                                        MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = wrapper(irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 2);
    const auto & r = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::StringPtr>(r));
    ASSERT_EQ(std::get<MIR::StringPtr>(r)->value, "x86_64");
}

TEST(machine_lower, in_array) {
    auto irlist = lower("x = [host_machine.cpu_family()]");
    auto info =
        MachineInfo(MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                                        MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = wrapper(irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & r = irlist->block->instructions.front();

    ASSERT_TRUE(std::holds_alternative<MIR::ArrayPtr>(r));
    const auto & a = std::get<MIR::ArrayPtr>(r)->value;

    ASSERT_EQ(a.size(), 1);
    ASSERT_TRUE(std::holds_alternative<MIR::StringPtr>(a[0]));
    ASSERT_EQ(std::get<MIR::StringPtr>(a[0])->value, "x86_64");
}

TEST(machine_lower, in_function_args) {
    auto irlist = lower("foo(host_machine.endian())");
    auto info =
        MachineInfo(MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                                        MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = wrapper(irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & r = irlist->block->instructions.front();

    ASSERT_TRUE(std::holds_alternative<MIR::FunctionCallPtr>(r));
    const auto & f = std::get<MIR::FunctionCallPtr>(r);

    ASSERT_EQ(f->pos_args.size(), 1);
    ASSERT_TRUE(std::holds_alternative<MIR::StringPtr>(f->pos_args[0]));
    ASSERT_EQ(std::get<MIR::StringPtr>(f->pos_args[0])->value, "little");
}

TEST(machine_lower, in_condition) {
    auto irlist = lower("if host_machine.cpu_family()\n x = 2\nendif");
    auto info =
        MachineInfo(MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                                        MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = wrapper(irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & obj =
        std::get<0>(std::get<MIR::BranchPtr>(irlist->block->instructions.back())->branches.at(0));
    ASSERT_TRUE(std::holds_alternative<MIR::StringPtr>(obj));
    ASSERT_EQ(std::get<MIR::StringPtr>(obj)->value, "x86_64");
}
