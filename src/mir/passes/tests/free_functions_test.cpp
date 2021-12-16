// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>

#include "arguments.hpp"
#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"
#include "toolchains/archiver.hpp"
#include "toolchains/common.hpp"
#include "toolchains/compilers/cpp/cpp.hpp"
#include "toolchains/linker.hpp"

#include "test_utils.hpp"

TEST(files, simple) {
    auto irlist = lower("x = files('foo.c')");

    const MIR::State::Persistant pstate{src_root, build_root};

    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Array>>(r));

    const auto & a = std::get<std::unique_ptr<MIR::Array>>(r)->value;
    ASSERT_EQ(a.size(), 1);

    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::File>>(a[0]));

    const auto & f = std::get<std::unique_ptr<MIR::File>>(a[0]);
    ASSERT_EQ(f->file.get_name(), "foo.c");
}

TEST(executable, simple) {
    auto irlist = lower("x = executable('exe', 'source.c', cpp_args : ['-Dfoo'])");

    MIR::State::Persistant pstate{src_root, build_root};
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Executable>>(r));

    const auto & e = std::get<std::unique_ptr<MIR::Executable>>(r)->value;
    ASSERT_EQ(e.name, "exe");
    ASSERT_TRUE(e.arguments.find(MIR::Toolchain::Language::CPP) != e.arguments.end());

    const auto & args = e.arguments.at(MIR::Toolchain::Language::CPP);
    ASSERT_EQ(args.size(), 1);

    const auto & a = args.front();
    ASSERT_EQ(a.type, MIR::Arguments::Type::DEFINE);
    ASSERT_EQ(a.value, "foo");
}

TEST(static_library, simple) {
    auto irlist = lower("x = static_library('exe', 'source.c', cpp_args : '-Dfoo')");

    MIR::State::Persistant pstate{src_root, build_root};
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::StaticLibrary>>(r));

    const auto & e = std::get<std::unique_ptr<MIR::StaticLibrary>>(r)->value;
    ASSERT_EQ(e.name, "exe");
    ASSERT_TRUE(e.arguments.find(MIR::Toolchain::Language::CPP) != e.arguments.end());

    const auto & args = e.arguments.at(MIR::Toolchain::Language::CPP);
    ASSERT_EQ(args.size(), 1);

    const auto & a = args.front();
    ASSERT_EQ(a.type, MIR::Arguments::Type::DEFINE);
    ASSERT_EQ(a.value, "foo");
}

TEST(project, valid) {
    auto irlist = lower("project('foo')");
    MIR::State::Persistant pstate{src_root, build_root};
    MIR::Passes::lower_project(&irlist, pstate);
    ASSERT_EQ(pstate.name, "foo");
}
