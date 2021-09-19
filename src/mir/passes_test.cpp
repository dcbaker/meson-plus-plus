// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>
#include <sstream>
#include <variant>

#include "arguments.hpp"
#include "ast_to_mir.hpp"
#include "driver.hpp"
#include "exceptions.hpp"
#include "lower.hpp"
#include "mir.hpp"
#include "passes.hpp"
#include "state/state.hpp"
#include "toolchains/archiver.hpp"
#include "toolchains/common.hpp"
#include "toolchains/compilers/cpp/cpp.hpp"
#include "toolchains/linker.hpp"

namespace {

static const std::filesystem::path src_root = "/home/test user/src/test project/";
static const std::filesystem::path build_root = "/home/test user/src/test project/builddir/";

std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in) {
    Frontend::Driver drv{};
    std::istringstream stream{in};
    auto src = src_root / "meson.build";
    drv.name = src;
    auto block = drv.parse(stream);
    return block;
}

MIR::BasicBlock lower(const std::string & in) {
    auto block = parse(in);
    auto ir = MIR::lower_ast(block);
    return ir;
}

} // namespace

TEST(branch_pruning, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nendif\n");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_EQ(irlist.instructions.size(), 2);
}

TEST(branch_pruning, next_block) {
    auto irlist = lower("x = 7\nif true\n x = 8\nendif\ny = x");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_NE(irlist.next, nullptr);
    ASSERT_EQ(irlist.next->instructions.size(), 1);
}

TEST(branch_pruning, if_else) {
    auto irlist = lower("x = 7\nif true\n x = 8\nelse\n x = 9\nendif\n");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_EQ(irlist.instructions.size(), 2);
}

TEST(branch_pruning, if_false) {
    auto irlist = lower("x = 7\nif false\n x = 8\nelse\n x = 9\n y = 2\nendif\n");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    // Using 3 here allows us to know that we went down the right path
    ASSERT_EQ(irlist.instructions.size(), 3);

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.front());
    ASSERT_EQ(first->value, 7);
    ASSERT_EQ(first->var.name, "x");

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.back());
    ASSERT_EQ(last->value, 2);
    ASSERT_EQ(last->var.name, "y");
}

TEST(join_blocks, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nelse\n x = 9\nendif\ny = x");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_EQ(irlist.instructions.size(), 2);
    ASSERT_NE(irlist.next, nullptr);

    ASSERT_EQ(irlist.next->instructions.size(), 1);

    progress = MIR::Passes::join_blocks(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_FALSE(irlist.condition.has_value());
    ASSERT_EQ(irlist.instructions.size(), 3);
    ASSERT_EQ(irlist.next, nullptr);
}

TEST(machine_lower, simple) {
    auto irlist = lower("x = 7\ny = host_machine.cpu_family()");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(&irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 2);
    const auto & r = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::String>>(r));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::String>>(r)->value, "x86_64");
}

TEST(machine_lower, in_array) {
    auto irlist = lower("x = [host_machine.cpu_family()]");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(&irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & r = irlist.instructions.front();

    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Array>>(r));
    const auto & a = std::get<std::unique_ptr<MIR::Array>>(r)->value;

    ASSERT_EQ(a.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::String>>(a[0]));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::String>>(a[0])->value, "x86_64");
}

TEST(machine_lower, in_function_args) {
    auto irlist = lower("foo(host_machine.endian())");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(&irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & r = irlist.instructions.front();

    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(r));
    const auto & f = std::get<std::unique_ptr<MIR::FunctionCall>>(r);

    ASSERT_EQ(f->pos_args.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::String>>(f->pos_args[0]));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::String>>(f->pos_args[0])->value, "little");
}

TEST(machine_lower, in_condtion) {
    auto irlist = lower("if host_machine.cpu_family()\n x = 2\nendif");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(&irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 0);

    const auto & con = irlist.condition;
    ASSERT_TRUE(con.has_value());
    const auto & obj = con.value().condition;
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::String>>(obj));
    ASSERT_EQ(std::get<std::unique_ptr<MIR::String>>(obj)->value, "x86_64");
}

TEST(insert_compiler, simple) {
    const std::vector<std::string> init{"null"};
    auto comp = std::make_unique<MIR::Toolchain::Compiler::CPP::Clang>(init);
    auto tc = std::make_shared<MIR::Toolchain::Toolchain>(
        std::move(comp),
        std::make_unique<MIR::Toolchain::Linker::Drivers::Gnu>(MIR::Toolchain::Linker::GnuBFD{init},
                                                               comp.get()),
        std::make_unique<MIR::Toolchain::Archiver::Gnu>(init));
    std::unordered_map<MIR::Toolchain::Language,
                       MIR::Machines::PerMachine<std::shared_ptr<MIR::Toolchain::Toolchain>>>
        tc_map{};
    tc_map[MIR::Toolchain::Language::CPP] =
        MIR::Machines::PerMachine<std::shared_ptr<MIR::Toolchain::Toolchain>>{tc};

    auto irlist = lower("x = meson.get_compiler('cpp')");
    bool progress = MIR::Passes::insert_compilers(&irlist, tc_map);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & e = irlist.instructions.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Compiler>>(e));

    const auto & c = std::get<std::unique_ptr<MIR::Compiler>>(e);
    ASSERT_EQ(c->toolchain->compiler->id(), "clang");
}

TEST(insert_compiler, unknown_language) {
    std::unordered_map<MIR::Toolchain::Language,
                       MIR::Machines::PerMachine<std::shared_ptr<MIR::Toolchain::Toolchain>>>
        tc_map{};

    auto irlist = lower("x = meson.get_compiler('cpp')");
    try {
        (void)MIR::Passes::insert_compilers(&irlist, tc_map);
        FAIL();
    } catch (Util::Exceptions::MesonException & e) {
        ASSERT_EQ(e.message, "No compiler for language");
    }
}

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

TEST(project, valid) {
    auto irlist = lower("project('foo')");
    MIR::State::Persistant pstate{src_root, build_root};
    MIR::Passes::lower_project(&irlist, pstate);
    ASSERT_EQ(pstate.name, "foo");
}

TEST(lower, trivial) {
    auto irlist = lower("project('foo')");
    MIR::State::Persistant pstate{src_root, build_root};
    MIR::Passes::lower_project(&irlist, pstate);
    MIR::lower(&irlist, pstate);
}

#if false
TEST(lower, simple_real) {
    auto irlist = lower(R"EOF(
        project('foo', 'c')

        t_files = files(
            'bar.c',
        )

        executable(
            'exe',
            t_files,
        )
    )EOF");
    MIR::State::Persistant pstate{src_root, build_root};
    MIR::Passes::lower_project(&irlist, pstate);
    MIR::lower(&irlist, pstate);
}
#endif
