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
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::Array>>(r));

    const auto & a = std::get<std::shared_ptr<MIR::Array>>(r)->value;
    ASSERT_EQ(a.size(), 1);

    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::File>>(a[0]));

    const auto & f = std::get<std::shared_ptr<MIR::File>>(a[0]);
    ASSERT_EQ(f->get_name(), "foo.c");
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
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::Executable>>(r));

    const auto & e = *std::get<std::shared_ptr<MIR::Executable>>(r);
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
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::StaticLibrary>>(r));

    const auto & e = *std::get<std::shared_ptr<MIR::StaticLibrary>>(r);
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

TEST(messages, simple) {
    auto irlist = lower("message('foo')");
    MIR::State::Persistant pstate{src_root, build_root};
    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Message>>(r));

    const auto & m = std::get<std::unique_ptr<MIR::Message>>(r);
    ASSERT_EQ(m->level, MIR::MessageLevel::MESSAGE);
    ASSERT_EQ(m->message, "foo");
}

TEST(messages, two_args) {
    auto irlist = lower("warning('foo', 'bar')");
    MIR::State::Persistant pstate{src_root, build_root};
    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Message>>(r));

    const auto & m = std::get<std::unique_ptr<MIR::Message>>(r);
    ASSERT_EQ(m->level, MIR::MessageLevel::WARN);
    ASSERT_EQ(m->message, "foo bar");
}

TEST(assert, simple) {
    auto irlist = lower("assert(false)");
    MIR::State::Persistant pstate{src_root, build_root};
    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Message>>(r));

    const auto & m = std::get<std::unique_ptr<MIR::Message>>(r);
    ASSERT_EQ(m->level, MIR::MessageLevel::ERROR);
    ASSERT_EQ(m->message, "Assertion failed: ");
}

TEST(find_program, found) {
    auto irlist = lower(R"EOF(
        x = find_program('sh')
        x.found()
    )EOF");
    MIR::Passes::ValueTable vt{};
    MIR::Passes::LastSeenTable lt{};
    MIR::Passes::PropTable pt{};
    MIR::Passes::ReplacementTable rt{};
    MIR::State::Persistant pstate{src_root, build_root};

    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::threaded_lowering(b, pstate); },
                 });
    bool progress = MIR::Passes::block_walker(
        &irlist,
        {
            [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, vt); },
            [&](MIR::BasicBlock * b) { return MIR::Passes::usage_numbering(b, lt); },
            [&](MIR::BasicBlock * b) { return MIR::Passes::constant_folding(b, rt); },
            [&](MIR::BasicBlock * b) { return MIR::Passes::constant_propogation(b, pt); },
            [&](MIR::BasicBlock * b) { return MIR::Passes::lower_program_objects(*b, pstate); },
        });

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 2);

    const auto & r = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::Boolean>>(r));

    const auto & m = std::get<std::shared_ptr<MIR::Boolean>>(r);
    ASSERT_EQ(m->value, true);
}

TEST(not, simple) {
    auto irlist = lower("not false");
    const MIR::State::Persistant pstate{src_root, build_root};
    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::Boolean>>(r));

    const auto & m = std::get<std::shared_ptr<MIR::Boolean>>(r);
    ASSERT_EQ(m->value, true);
}

TEST(neg, simple) {
    auto irlist = lower("-5");
    const MIR::State::Persistant pstate{src_root, build_root};
    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::Number>>(r));

    const auto & m = std::get<std::shared_ptr<MIR::Number>>(r);
    ASSERT_EQ(m->value, -5);
}

TEST(custom_target, simple) {
    auto irlist =
        lower("custom_target('foo', input : 'bar.in', output : 'bar.cpp', command : 'thing')");

    const MIR::State::Persistant pstate{src_root, build_root};

    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::CustomTarget>>(r));

    const auto & ct = *std::get<std::shared_ptr<MIR::CustomTarget>>(r);
    ASSERT_EQ(ct.name, "foo");
    ASSERT_EQ(ct.command, std::vector<std::string>{"thing"});
}

static inline bool _test_equality(const std::string & expr) {
    auto irlist = lower(expr);

    const MIR::State::Persistant pstate{src_root, build_root};

    bool progress = MIR::Passes::lower_free_functions(&irlist, pstate);
    const auto & r = irlist.instructions.front();
    const auto & value = *std::get<std::shared_ptr<MIR::Boolean>>(r);
    return value.value;
}

TEST(ne, number_false) { ASSERT_FALSE(_test_equality("1 != 1")); }
TEST(ne, number_true) { ASSERT_TRUE(_test_equality("1 != 5")); }
TEST(eq, number_false) { ASSERT_FALSE(_test_equality("1 == 5")); }
TEST(eq, number_true) { ASSERT_TRUE(_test_equality("1 == 1")); }

TEST(ne, string_false) { ASSERT_FALSE(_test_equality("'' != ''")); }
TEST(ne, string_true) { ASSERT_TRUE(_test_equality("'' != 'foo'")); }
TEST(eq, string_false) { ASSERT_FALSE(_test_equality("'foo' == 'bar'")); }
TEST(eq, string_true) { ASSERT_TRUE(_test_equality("'foo' == 'foo'")); }

TEST(ne, boolean_false) { ASSERT_FALSE(_test_equality("false != false")); }
TEST(ne, boolean_true) { ASSERT_TRUE(_test_equality("false != true")); }
TEST(eq, boolean_false) { ASSERT_FALSE(_test_equality("false == true")); }
TEST(eq, boolean_true) { ASSERT_TRUE(_test_equality("false == false")); }
