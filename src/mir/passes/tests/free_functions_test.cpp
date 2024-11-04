// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "arguments.hpp"
#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"
#include "test_utils.hpp"
#include "toolchains/archiver.hpp"
#include "toolchains/common.hpp"
#include "toolchains/compilers/cpp/cpp.hpp"
#include "toolchains/linker.hpp"

#include <gtest/gtest.h>

TEST(files, simple) {
    auto irlist = lower("x = files('foo.c')");

    const MIR::State::Persistant pstate = make_pstate();

    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Array>(*r.obj_ptr));

    const auto & a = std::get<MIR::Array>(*r.obj_ptr).value;
    ASSERT_EQ(a.size(), 1);

    ASSERT_TRUE(std::holds_alternative<MIR::File>(*a[0].obj_ptr));

    const auto & f = std::get<MIR::File>(*a[0].obj_ptr);
    ASSERT_EQ(f.get_name(), "foo.c");
}

TEST(executable, simple) {
    auto irlist = lower("x = executable('exe', 'source.c', cpp_args : ['-Dfoo'])");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Executable>(*r.obj_ptr));

    const auto & e = std::get<MIR::Executable>(*r.obj_ptr);
    ASSERT_EQ(e.name, "exe");
    ASSERT_TRUE(e.arguments.find(MIR::Toolchain::Language::CPP) != e.arguments.end());

    const auto & args = e.arguments.at(MIR::Toolchain::Language::CPP);
    ASSERT_EQ(args.size(), 1);

    const auto & a = args.front();
    ASSERT_EQ(a.type(), MIR::Arguments::Type::DEFINE);
    ASSERT_EQ(a.value(), "foo");
}

TEST(static_library, simple) {
    auto irlist = lower("x = static_library('exe', 'source.c', cpp_args : '-Dfoo')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::StaticLibrary>(*r.obj_ptr));

    const auto & e = std::get<MIR::StaticLibrary>(*r.obj_ptr);
    ASSERT_EQ(e.name, "exe");
    ASSERT_TRUE(e.arguments.find(MIR::Toolchain::Language::CPP) != e.arguments.end());

    const auto & args = e.arguments.at(MIR::Toolchain::Language::CPP);
    ASSERT_EQ(args.size(), 1);

    const auto & a = args.front();
    ASSERT_EQ(a.type(), MIR::Arguments::Type::DEFINE);
    ASSERT_EQ(a.value(), "foo");
}

TEST(project, valid) {
    auto irlist = lower("project('foo')");
    MIR::State::Persistant pstate = make_pstate();
    MIR::Passes::lower_project(irlist, pstate);
    ASSERT_EQ(pstate.name, "foo");
}

TEST(project, vararg_array) {
    auto irlist = lower("project('foo', ['cpp'])");
    MIR::State::Persistant pstate = make_pstate();
    MIR::Passes::lower_project(irlist, pstate);
    ASSERT_EQ(pstate.name, "foo");
    ASSERT_TRUE(pstate.toolchains.find(MIR::Toolchain::Language::CPP) != pstate.toolchains.end());
}

TEST(messages, simple) {
    auto irlist = lower("message('foo')");
    MIR::State::Persistant pstate = make_pstate();
    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Message>(*r.obj_ptr));

    const auto & m = std::get<MIR::Message>(*r.obj_ptr);
    ASSERT_EQ(m.level, MIR::MessageLevel::MESSAGE);
    ASSERT_EQ(m.message, "foo");
}

TEST(messages, two_args) {
    auto irlist = lower("warning('foo', 'bar')");
    MIR::State::Persistant pstate = make_pstate();
    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Message>(*r.obj_ptr));

    const auto & m = std::get<MIR::Message>(*r.obj_ptr);
    ASSERT_EQ(m.level, MIR::MessageLevel::WARN);
    ASSERT_EQ(m.message, "foo bar");
}

TEST(assert, simple) {
    auto irlist = lower("assert(false)");
    MIR::State::Persistant pstate = make_pstate();
    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Message>(*r.obj_ptr));

    const auto & m = std::get<MIR::Message>(*r.obj_ptr);
    ASSERT_EQ(m.level, MIR::MessageLevel::ERROR);
    ASSERT_EQ(m.message, "Assertion failed: ");
}

TEST(find_program, found) {
    auto irlist = lower(R"EOF(
        x = find_program('sh')
        x.found()
    )EOF");
    MIR::State::Persistant pstate = make_pstate();

    MIR::Passes::block_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          [&](std::shared_ptr<MIR::BasicBlock> b) {
                                              return MIR::Passes::threaded_lowering(b, pstate);
                                          },
                                      });
    bool progress = MIR::Passes::block_walker(
        irlist, {
                    MIR::Passes::ConstantFolding{},
                    MIR::Passes::ConstantPropagation{},
                    [&](std::shared_ptr<MIR::BasicBlock> b) {
                        return MIR::Passes::lower_program_objects(b, pstate);
                    },
                });

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 2);

    const auto & r = irlist->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::Boolean>(*r.obj_ptr));

    const auto & m = std::get<MIR::Boolean>(*r.obj_ptr);
    ASSERT_EQ(m.value, true);
}

TEST(not, simple) {
    auto irlist = lower("not false");
    const MIR::State::Persistant pstate = make_pstate();
    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::Boolean>(*r.obj_ptr));

    const auto & m = std::get<MIR::Boolean>(*r.obj_ptr);
    ASSERT_EQ(m.value, true);
}

TEST(neg, simple) {
    auto irlist = lower("-5");
    const MIR::State::Persistant pstate = make_pstate();
    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::Number>(*r.obj_ptr));

    const auto & m = std::get<MIR::Number>(*r.obj_ptr);
    ASSERT_EQ(m.value, -5);
}

TEST(custom_target, simple) {
    auto irlist =
        lower("custom_target('foo', input : 'bar.in', output : 'bar.cpp', command : 'thing')");

    const MIR::State::Persistant pstate = make_pstate();

    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::CustomTarget>(*r.obj_ptr));

    const auto & ct = std::get<MIR::CustomTarget>(*r.obj_ptr);
    ASSERT_EQ(ct.name, "foo");
    ASSERT_EQ(ct.command, std::vector<std::string>{"thing"});
}

static inline bool test_equality(const std::string & expr) {
    auto irlist = lower(expr);

    const MIR::State::Persistant pstate = make_pstate();

    MIR::Passes::lower_free_functions(irlist, pstate);
    const auto & r = irlist->instructions.front();
    const auto & value = std::get<MIR::Boolean>(*r.obj_ptr);
    return value.value;
}

TEST(ne, number_false) { ASSERT_FALSE(test_equality("1 != 1")); }
TEST(ne, number_true) { ASSERT_TRUE(test_equality("1 != 5")); }
TEST(eq, number_false) { ASSERT_FALSE(test_equality("1 == 5")); }
TEST(eq, number_true) { ASSERT_TRUE(test_equality("1 == 1")); }

TEST(ne, string_false) { ASSERT_FALSE(test_equality("'' != ''")); }
TEST(ne, string_true) { ASSERT_TRUE(test_equality("'' != 'foo'")); }
TEST(eq, string_false) { ASSERT_FALSE(test_equality("'foo' == 'bar'")); }
TEST(eq, string_true) { ASSERT_TRUE(test_equality("'foo' == 'foo'")); }

TEST(ne, boolean_false) { ASSERT_FALSE(test_equality("false != false")); }
TEST(ne, boolean_true) { ASSERT_TRUE(test_equality("false != true")); }
TEST(eq, boolean_false) { ASSERT_FALSE(test_equality("false == true")); }
TEST(eq, boolean_true) { ASSERT_TRUE(test_equality("false == false")); }

TEST(version_compare, simple) {
    auto irlist = lower("'3.6'.version_compare('< 3.7')");

    MIR::State::Persistant pstate = make_pstate();

    bool progress = MIR::Passes::lower_string_objects(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Boolean>(*r.obj_ptr));

    const auto & ct = std::get<MIR::Boolean>(*r.obj_ptr);
    ASSERT_TRUE(ct.value);
}

TEST(declare_dependency, string_include_dirs) {
    auto irlist = lower("x = declare_dependency(include_directories : 'foo')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Dependency>(*r.obj_ptr));

    const auto & d = std::get<MIR::Dependency>(*r.obj_ptr);
    ASSERT_EQ(d.arguments.size(), 1);
    ASSERT_EQ(d.arguments[0].value(), "foo");
}

TEST(declare_dependency, compile_args) {
    auto irlist = lower("x = declare_dependency(compile_args : '-Dfoo')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Dependency>(*r.obj_ptr));

    const auto & d = std::get<MIR::Dependency>(*r.obj_ptr);
    ASSERT_EQ(d.arguments.size(), 1);
    ASSERT_EQ(d.arguments[0].value(), "foo");
    ASSERT_EQ(d.arguments[0].type(), MIR::Arguments::Type::DEFINE);
}

TEST(declare_dependency, recursive) {
    auto irlist =
        lower("x = declare_dependency(dependencies : declare_dependency(compile_args : '-Dfoo'))");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = true;
    while (progress) {
        progress = MIR::Passes::lower_free_functions(irlist, pstate);
    }
    ASSERT_EQ(irlist->instructions.size(), 1);

    const auto & r = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::Dependency>(*r.obj_ptr));

    const auto & d = std::get<MIR::Dependency>(*r.obj_ptr);
    ASSERT_EQ(d.arguments.size(), 1);
    ASSERT_EQ(d.arguments[0].value(), "foo");
    ASSERT_EQ(d.arguments[0].type(), MIR::Arguments::Type::DEFINE);
}

TEST(add_project_arguments, simple) {
    auto irlist = lower("add_project_arguments('-DFOO', language : 'cpp')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    const bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);

    const auto & ir = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArguments>(*ir.obj_ptr));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArguments>(*ir.obj_ptr);
    ASSERT_TRUE(add.arguments.find(Language::CPP) != add.arguments.end());
    const auto & args = add.arguments.at(Language::CPP);
    EXPECT_FALSE(add.is_global);
    ASSERT_EQ(add.arguments.size(), 1);
    const auto & arg = args.front();

    EXPECT_EQ(arg.type(), MIR::Arguments::Type::DEFINE);
    EXPECT_EQ(arg.value(), "FOO");
}

TEST(add_project_link_arguments, simple) {
    auto irlist = lower("add_project_link_arguments('-Wl,-foo', language : 'cpp')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    const bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);

    const auto & ir = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArguments>(*ir.obj_ptr));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArguments>(*ir.obj_ptr);
    ASSERT_TRUE(add.arguments.find(Language::CPP) != add.arguments.end());
    const auto & args = add.arguments.at(Language::CPP);
    EXPECT_FALSE(add.is_global);
    ASSERT_EQ(add.arguments.size(), 1);
    const auto & arg = args.front();

    EXPECT_EQ(arg.type(), MIR::Arguments::Type::RAW_LINK);
    EXPECT_EQ(arg.value(), "-Wl,-foo");
}

TEST(add_global_arguments, simple) {
    auto irlist = lower("add_global_arguments('-Wno-foo', language : 'cpp')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    const bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);

    const auto & ir = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArguments>(*ir.obj_ptr));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArguments>(*ir.obj_ptr);
    ASSERT_TRUE(add.arguments.find(Language::CPP) != add.arguments.end());
    const auto & args = add.arguments.at(Language::CPP);
    EXPECT_TRUE(add.is_global);
    ASSERT_EQ(add.arguments.size(), 1);
    const auto & arg = args.front();

    EXPECT_EQ(arg.type(), MIR::Arguments::Type::RAW);
    EXPECT_EQ(arg.value(), "-Wno-foo");
}

TEST(add_global_link_arguments, simple) {
    auto irlist = lower("add_global_link_arguments('-Lbar', language : 'cpp')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    const bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
    ASSERT_TRUE(progress);

    const auto & ir = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArguments>(*ir.obj_ptr));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArguments>(*ir.obj_ptr);
    ASSERT_TRUE(add.arguments.find(Language::CPP) != add.arguments.end());
    const auto & args = add.arguments.at(Language::CPP);
    EXPECT_TRUE(add.is_global);
    ASSERT_EQ(add.arguments.size(), 1);
    const auto & arg = args.front();

    EXPECT_EQ(arg.type(), MIR::Arguments::Type::LINK_SEARCH);
    EXPECT_EQ(arg.value(), "bar");
}

TEST(add_global_link_arguments, combine) {
    auto irlist = lower(R"EOF(
        add_global_link_arguments('-Lbar', language : 'cpp')
        add_global_link_arguments('-lfoo', language : 'cpp')
    )EOF");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    MIR::Passes::Printer printer{};
    printer(irlist);
    printer.increment();

    {
        const bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
        printer(irlist);
        printer.increment();
        ASSERT_TRUE(progress);
    }
    {
        const bool progress = MIR::Passes::combine_add_arguments(irlist);
        printer(irlist);
        ASSERT_TRUE(progress);
    }

    ASSERT_EQ(irlist->instructions.size(), 1);
    const auto & ir = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArguments>(*ir.obj_ptr));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArguments>(*ir.obj_ptr);
    ASSERT_TRUE(add.arguments.find(Language::CPP) != add.arguments.end());
    const auto & args = add.arguments.at(Language::CPP);
    EXPECT_TRUE(add.is_global);
    ASSERT_EQ(args.size(), 2);

    {
        const auto & arg = args.front();
        EXPECT_EQ(arg.type(), MIR::Arguments::Type::LINK_SEARCH);
        EXPECT_EQ(arg.value(), "bar");
    }
    {
        const auto & arg = args.back();
        EXPECT_EQ(arg.type(), MIR::Arguments::Type::LINK);
        EXPECT_EQ(arg.value(), "foo");
    }
}

TEST(add_global_link_arguments, combine_complex) {
    auto irlist = lower(R"EOF(
        add_global_link_arguments('-Lbar', language : 'cpp')
        add_global_arguments('-Dfoo', language : 'cpp')
    )EOF");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    MIR::Passes::Printer printer{};
    printer(irlist);
    printer.increment();

    {
        const bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
        printer(irlist);
        printer.increment();
        ASSERT_TRUE(progress);
    }
    {
        const bool progress = MIR::Passes::combine_add_arguments(irlist);
        printer(irlist);
        ASSERT_TRUE(progress);
    }

    ASSERT_EQ(irlist->instructions.size(), 1);
    const auto & ir = irlist->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArguments>(*ir.obj_ptr));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArguments>(*ir.obj_ptr);
    ASSERT_TRUE(add.arguments.find(Language::CPP) != add.arguments.end());
    const auto & args = add.arguments.at(Language::CPP);
    EXPECT_TRUE(add.is_global);
    ASSERT_EQ(args.size(), 2);

    {
        const auto & arg = args.front();
        EXPECT_EQ(arg.type(), MIR::Arguments::Type::LINK_SEARCH);
        EXPECT_EQ(arg.value(), "bar");
    }
    {
        const auto & arg = args.back();
        EXPECT_EQ(arg.type(), MIR::Arguments::Type::DEFINE);
        EXPECT_EQ(arg.value(), "foo");
    }
}

TEST(add_global_link_arguments, dont_combine) {
    auto irlist = lower(R"EOF(
        add_global_link_arguments('-Lbar', language : 'cpp')
        add_project_link_arguments('-lfoo', language : 'cpp')
    )EOF");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    MIR::Passes::Printer printer{};
    printer(irlist);
    printer.increment();

    {
        const bool progress = MIR::Passes::lower_free_functions(irlist, pstate);
        printer(irlist);
        printer.increment();
        EXPECT_TRUE(progress);
    }
    {
        const bool progress = MIR::Passes::combine_add_arguments(irlist);
        printer(irlist);
        EXPECT_FALSE(progress);
    }

    ASSERT_EQ(irlist->instructions.size(), 2);
}
