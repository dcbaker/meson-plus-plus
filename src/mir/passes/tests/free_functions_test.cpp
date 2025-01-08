// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

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
#include <tuple>

namespace {

bool wrapper(std::shared_ptr<MIR::CFGNode> node, const MIR::State::Persistant & pstate) {
    return MIR::Passes::instruction_walker(*node, {[&pstate](const MIR::Object & inst) {
        return MIR::Passes::lower_free_functions(inst, pstate);
    }});
}

} // namespace

TEST(files, simple) {
    auto irlist = lower("x = files('foo.c')");

    const MIR::State::Persistant pstate = make_pstate();
    bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::ArrayPtr>(r));

    const auto & a = std::get<MIR::ArrayPtr>(r)->value;
    ASSERT_EQ(a.size(), 1);

    ASSERT_TRUE(std::holds_alternative<MIR::FilePtr>(a[0]));

    const auto & f = std::get<MIR::FilePtr>(a[0]);
    ASSERT_EQ(f->get_name(), "foo.c");
}

TEST(executable, simple) {
    auto irlist = lower("x = executable('exe', 'source.c', cpp_args : ['-Dfoo'])");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::ExecutablePtr>(r));

    const auto & e = std::get<MIR::ExecutablePtr>(r);
    ASSERT_EQ(e->name, "exe");
    ASSERT_TRUE(e->arguments.find(MIR::Toolchain::Language::CPP) != e->arguments.end());

    const auto & args = e->arguments.at(MIR::Toolchain::Language::CPP);
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

    bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::StaticLibraryPtr>(r));

    const auto & e = std::get<MIR::StaticLibraryPtr>(r);
    ASSERT_EQ(e->name, "exe");
    ASSERT_TRUE(e->arguments.find(MIR::Toolchain::Language::CPP) != e->arguments.end());

    const auto & args = e->arguments.at(MIR::Toolchain::Language::CPP);
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
    bool progress = wrapper(irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::MessagePtr>(r));

    const auto & m = std::get<MIR::MessagePtr>(r);
    ASSERT_EQ(m->level, MIR::MessageLevel::MESSAGE);
    ASSERT_EQ(m->message, "foo");
}

TEST(messages, two_args) {
    auto irlist = lower("warning('foo', 'bar')");
    MIR::State::Persistant pstate = make_pstate();
    bool progress = wrapper(irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::MessagePtr>(r));

    const auto & m = std::get<MIR::MessagePtr>(r);
    ASSERT_EQ(m->level, MIR::MessageLevel::WARN);
    ASSERT_EQ(m->message, "foo bar");
}

TEST(assert, simple) {
    auto irlist = lower("assert(false)");
    MIR::State::Persistant pstate = make_pstate();
    bool progress = wrapper(irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::MessagePtr>(r));

    const auto & m = std::get<MIR::MessagePtr>(r);
    ASSERT_EQ(m->level, MIR::MessageLevel::ERROR);
    ASSERT_EQ(m->message, "Assertion failed: ");
}

TEST(find_program, found) {
    auto irlist = lower(R"EOF(
        x = find_program('sh')
        x.found()
    )EOF");
    MIR::State::Persistant pstate = make_pstate();

    MIR::Passes::graph_walker(irlist, {
                                          MIR::Passes::GlobalValueNumbering{},
                                          [&](std::shared_ptr<MIR::CFGNode> b) {
                                              return MIR::Passes::threaded_lowering(b, pstate);
                                          },
                                      });
    bool progress = MIR::Passes::graph_walker(
        irlist,
        {
            MIR::Passes::ConstantFolding{},
            MIR::Passes::ConstantPropagation{},
            [&pstate](std::shared_ptr<MIR::CFGNode> b) {
                return MIR::Passes::instruction_walker(*b, {[&pstate](const MIR::Object & inst) {
                    return MIR::Passes::lower_program_objects(inst, pstate);
                }});
            },
        });

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 2);

    const auto & r = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::BooleanPtr>(r));

    const auto & m = std::get<MIR::BooleanPtr>(r);
    ASSERT_EQ(m->value, true);
}

TEST(not, simple) {
    auto irlist = lower("not false");
    const MIR::State::Persistant pstate = make_pstate();
    bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::BooleanPtr>(r));

    const auto & m = std::get<MIR::BooleanPtr>(r);
    ASSERT_EQ(m->value, true);
}

TEST(neg, simple) {
    auto irlist = lower("-5");
    const MIR::State::Persistant pstate = make_pstate();
    bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.back();
    ASSERT_TRUE(std::holds_alternative<MIR::NumberPtr>(r));

    const auto & m = std::get<MIR::NumberPtr>(r);
    ASSERT_EQ(m->value, -5);
}

TEST(custom_target, simple) {
    auto irlist =
        lower("custom_target('foo', input : 'bar.in', output : 'bar.cpp', command : 'thing')");

    const MIR::State::Persistant pstate = make_pstate();

    bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::CustomTargetPtr>(r));

    const auto & ct = std::get<MIR::CustomTargetPtr>(r);
    ASSERT_EQ(ct->name, "foo");
    ASSERT_EQ(ct->command, std::vector<std::string>{"thing"});
}

class TestEquality : public ::testing::TestWithParam<std::tuple<std::string, bool>> {};

TEST_P(TestEquality, compare) {
    const auto & [expr, expected] = GetParam();

    auto irlist = lower(expr);

    const MIR::State::Persistant pstate = make_pstate();

    const bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    const MIR::Object & r = irlist->block->instructions.front();
    const auto value = std::get<MIR::BooleanPtr>(r);
    ASSERT_EQ(value->value, expected);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    Number, TestEquality,
    ::testing::Values(
        std::tuple("1 != 1", false),
        std::tuple("1 != 5", true),
        std::tuple("1 == 1", true),
        std::tuple("1 == 5", false)
    ));

INSTANTIATE_TEST_SUITE_P(
    String, TestEquality,
    ::testing::Values(
        std::tuple("'' != ''", false),
        std::tuple("'' != 'foo'", true),
        std::tuple("'foo' == 'foo'", true),
        std::tuple("'foo' == 'bar'", false)
    ));

INSTANTIATE_TEST_SUITE_P(
    Boolean, TestEquality,
    ::testing::Values(
        std::tuple("false != false", false),
        std::tuple("true != false", true),
        std::tuple("false == false", true),
        std::tuple("false == true", false)
    ));
// clang-format on

TEST(version_compare, simple) {
    auto irlist = lower("'3.6'.version_compare('< 3.7')");

    MIR::State::Persistant pstate = make_pstate();

    bool progress = MIR::Passes::instruction_walker(*irlist, {[&pstate](const MIR::Object & inst) {
        return MIR::Passes::lower_string_objects(inst, pstate);
    }});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::BooleanPtr>(r));

    const auto & ct = std::get<MIR::BooleanPtr>(r);
    ASSERT_TRUE(ct->value);
}

TEST(declare_dependency, string_include_dirs) {
    auto irlist = lower("x = declare_dependency(include_directories : 'foo')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::DependencyPtr>(r));

    const auto & d = std::get<MIR::DependencyPtr>(r);
    ASSERT_EQ(d->arguments.size(), 1);
    ASSERT_EQ(d->arguments[0].value(), "foo");
}

TEST(declare_dependency, compile_args) {
    auto irlist = lower("x = declare_dependency(compile_args : '-Dfoo')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::DependencyPtr>(r));

    const auto & d = std::get<MIR::DependencyPtr>(r);
    ASSERT_EQ(d->arguments.size(), 1);
    ASSERT_EQ(d->arguments[0].value(), "foo");
    ASSERT_EQ(d->arguments[0].type(), MIR::Arguments::Type::DEFINE);
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
        progress = wrapper(irlist, pstate);
    }
    ASSERT_EQ(irlist->block->instructions.size(), 1);

    const auto & r = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::DependencyPtr>(r));

    const auto & d = std::get<MIR::DependencyPtr>(r);
    ASSERT_EQ(d->arguments.size(), 1);
    ASSERT_EQ(d->arguments[0].value(), "foo");
    ASSERT_EQ(d->arguments[0].type(), MIR::Arguments::Type::DEFINE);
}

TEST(add_project_arguments, simple) {
    auto irlist = lower("add_project_arguments('-DFOO', language : 'cpp')");

    MIR::State::Persistant pstate = make_pstate();
    pstate.toolchains[MIR::Toolchain::Language::CPP] =
        std::make_shared<MIR::Toolchain::Toolchain>(MIR::Toolchain::get_toolchain(
            MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD));

    const bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);

    const auto & ir = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArgumentsPtr>(ir));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArgumentsPtr>(ir);
    ASSERT_TRUE(add->arguments.find(Language::CPP) != add->arguments.end());
    const auto & args = add->arguments.at(Language::CPP);
    EXPECT_FALSE(add->is_global);
    ASSERT_EQ(add->arguments.size(), 1);
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

    const bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);

    const auto & ir = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArgumentsPtr>(ir));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArgumentsPtr>(ir);
    ASSERT_TRUE(add->arguments.find(Language::CPP) != add->arguments.end());
    const auto & args = add->arguments.at(Language::CPP);
    EXPECT_FALSE(add->is_global);
    ASSERT_EQ(add->arguments.size(), 1);
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

    const bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);

    const auto & ir = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArgumentsPtr>(ir));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArgumentsPtr>(ir);
    ASSERT_TRUE(add->arguments.find(Language::CPP) != add->arguments.end());
    const auto & args = add->arguments.at(Language::CPP);
    EXPECT_TRUE(add->is_global);
    ASSERT_EQ(add->arguments.size(), 1);
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

    const bool progress = wrapper(irlist, pstate);
    ASSERT_TRUE(progress);

    const auto & ir = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArgumentsPtr>(ir));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArgumentsPtr>(ir);
    ASSERT_TRUE(add->arguments.find(Language::CPP) != add->arguments.end());
    const auto & args = add->arguments.at(Language::CPP);
    EXPECT_TRUE(add->is_global);
    ASSERT_EQ(add->arguments.size(), 1);
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
        const bool progress = wrapper(irlist, pstate);
        printer(irlist);
        printer.increment();
        ASSERT_TRUE(progress);
    }
    {
        const bool progress = MIR::Passes::combine_add_arguments(irlist);
        printer(irlist);
        ASSERT_TRUE(progress);
    }

    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & ir = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArgumentsPtr>(ir));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArgumentsPtr>(ir);
    ASSERT_TRUE(add->arguments.find(Language::CPP) != add->arguments.end());
    const auto & args = add->arguments.at(Language::CPP);
    EXPECT_TRUE(add->is_global);
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
        const bool progress = wrapper(irlist, pstate);
        printer(irlist);
        printer.increment();
        ASSERT_TRUE(progress);
    }
    {
        const bool progress = MIR::Passes::combine_add_arguments(irlist);
        printer(irlist);
        ASSERT_TRUE(progress);
    }

    ASSERT_EQ(irlist->block->instructions.size(), 1);
    const auto & ir = irlist->block->instructions.front();
    ASSERT_TRUE(std::holds_alternative<MIR::AddArgumentsPtr>(ir));

    using MIR::Toolchain::Language;

    const auto & add = std::get<MIR::AddArgumentsPtr>(ir);
    ASSERT_TRUE(add->arguments.find(Language::CPP) != add->arguments.end());
    const auto & args = add->arguments.at(Language::CPP);
    EXPECT_TRUE(add->is_global);
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
        const bool progress = wrapper(irlist, pstate);
        printer(irlist);
        printer.increment();
        EXPECT_TRUE(progress);
    }
    {
        const bool progress = MIR::Passes::combine_add_arguments(irlist);
        printer(irlist);
        EXPECT_FALSE(progress);
    }

    ASSERT_EQ(irlist->block->instructions.size(), 2);
}
