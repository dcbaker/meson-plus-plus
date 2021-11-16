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
#include "passes/private.hpp"
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
    const MIR::State::Persistant pstate{src_root, build_root};
    auto ir = MIR::lower_ast(block, pstate);
    return ir;
}

inline bool is_bb(const MIR::NextType & next) {
    return std::holds_alternative<std::shared_ptr<MIR::BasicBlock>>(next);
}

inline std::shared_ptr<MIR::BasicBlock> get_bb(const MIR::NextType & next) {
    return std::get<std::shared_ptr<MIR::BasicBlock>>(next);
}

inline bool is_con(const MIR::NextType & next) {
    return std::holds_alternative<std::unique_ptr<MIR::Condition>>(next);
}

inline const std::unique_ptr<MIR::Condition> & get_con(const MIR::NextType & next) {
    return std::get<std::unique_ptr<MIR::Condition>>(next);
}

inline bool is_empty(const MIR::NextType & next) {
    return std::holds_alternative<std::monostate>(next);
}

} // namespace

TEST(flatten, basic) {
    auto irlist = lower("func(['a', ['b', ['c']], 'd'])");
    MIR::State::Persistant pstate{src_root, build_root};
    bool progress = MIR::Passes::flatten(&irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();

    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(r));
    const auto & f = std::get<std::unique_ptr<MIR::FunctionCall>>(r);
    ASSERT_EQ(f->name, "func");
    ASSERT_EQ(f->pos_args.size(), 1);

    const auto & arg = f->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Array>>(arg));
    const auto & arr = std::get<std::unique_ptr<MIR::Array>>(arg)->value;

    ASSERT_EQ(arr.size(), 4);
}

TEST(flatten, already_flat) {
    auto irlist = lower("func(['a', 'd'])");
    MIR::State::Persistant pstate{src_root, build_root};
    bool progress = MIR::Passes::flatten(&irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();

    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(r));
    const auto & f = std::get<std::unique_ptr<MIR::FunctionCall>>(r);
    ASSERT_EQ(f->name, "func");
    ASSERT_EQ(f->pos_args.size(), 1);

    const auto & arg = f->pos_args.front();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Array>>(arg));
    const auto & arr = std::get<std::unique_ptr<MIR::Array>>(arg)->value;

    ASSERT_EQ(arr.size(), 2);
}

TEST(flatten, mixed_args) {
    auto irlist = lower("project('foo', ['a', ['d']])");
    MIR::State::Persistant pstate{src_root, build_root};
    bool progress = MIR::Passes::flatten(&irlist, pstate);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    const auto & r = irlist.instructions.front();

    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(r));
    const auto & f = std::get<std::unique_ptr<MIR::FunctionCall>>(r);
    ASSERT_EQ(f->pos_args.size(), 2);

    const auto & arg = f->pos_args.back();
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Array>>(arg));
    const auto & arr = std::get<std::unique_ptr<MIR::Array>>(arg)->value;

    ASSERT_EQ(arr.size(), 2);
}

TEST(branch_pruning, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nendif\n");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist.next));
    const auto & next = get_bb(irlist.next);
    ASSERT_FALSE(is_con(next->next));
    ASSERT_EQ(next->instructions.size(), 1);
}

TEST(branch_pruning, next_block) {
    auto irlist = lower(R"EOF(
        x = 7
        if true
          x = 8
        endif
        y = x
        )EOF");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);
    ASSERT_TRUE(is_bb(irlist.next));

    const auto & next = get_bb(irlist.next);
    ASSERT_EQ(next->instructions.size(), 1);
    ASSERT_EQ(next->parents.size(), 1);
    ASSERT_TRUE(next->parents.count(&irlist));

    ASSERT_EQ(get_bb(next->next)->parents.count(next.get()), 1);
}

TEST(branch_pruning, if_else) {
    auto irlist = lower(R"EOF(
        x = 7
        if true
          x = 8
        else
          x = 9
        endif
        )EOF");
    bool progress = MIR::Passes::branch_pruning(&irlist);

    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist.next));
    const auto & next = get_bb(irlist.next);
    ASSERT_EQ(next->instructions.size(), 1);

    ASSERT_TRUE(is_bb(next->next));
    ASSERT_TRUE(get_bb(next->next)->instructions.empty());
}

TEST(branch_pruning, if_false) {
    auto irlist = lower(R"EOF(
        x = 7
        if false
          x = 8
        else
          x = 9
          y = 2
        endif
        )EOF");
    bool progress = MIR::Passes::block_walker(&irlist, {MIR::Passes::branch_pruning});
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist.next));
    const auto & next = get_bb(irlist.next);
    ASSERT_EQ(next->instructions.size(), 2);

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(next->instructions.front());
    ASSERT_EQ(first->value, 9);
    ASSERT_EQ(first->var.name, "x");

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(next->instructions.back());
    ASSERT_EQ(last->value, 2);
    ASSERT_EQ(last->var.name, "y");
}

TEST(join_blocks, simple) {
    auto irlist = lower("x = 7\nif true\n x = 8\nelse\n x = 9\nendif\ny = x");
    bool progress = MIR::Passes::branch_pruning(&irlist);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);

    ASSERT_TRUE(is_bb(irlist.next));
    const auto & next = get_bb(irlist.next);
    ASSERT_EQ(next->instructions.size(), 1);

    progress = MIR::Passes::block_walker(&irlist, {MIR::Passes::join_blocks});
    ASSERT_TRUE(progress);
    ASSERT_TRUE(is_empty(irlist.next));
    ASSERT_EQ(irlist.instructions.size(), 3);
}

TEST(join_blocks, nested_if_elif_else) {
    auto irlist = lower(R"EOF(
        x = 7
        if false
          x = 8
        elif A
          x = 16
        else
          if Q
            y = 7
          else
            y = 9
          endif

          if X
            x = 99
          endif
          x = 9
        endif
        y = x
        z = y
        )EOF");
    bool progress = MIR::Passes::block_walker(&irlist, {
                                                           MIR::Passes::branch_pruning,
                                                           MIR::Passes::join_blocks,
                                                       });
    ASSERT_TRUE(progress);

    // Check that the parents of the final block are correct
    const auto & con1 = get_con(irlist.next);
    const auto & bb1 = con1->if_true;

    const auto & fin = get_bb(bb1->next);
    ASSERT_EQ(fin->instructions.size(), 2);
    ASSERT_TRUE(fin->parents.count(bb1.get()));
    ASSERT_EQ(fin->parents.size(), 2);
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

    ASSERT_TRUE(is_con(irlist.next));
    const auto & con = get_con(irlist.next);
    const auto & obj = con->condition;
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
        std::string m{e.what()};
        ASSERT_EQ(m, "No compiler for language");
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

TEST(value_numbering, simple) {
    auto irlist = lower(R"EOF(
        x = 7
        x = 8
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::value_numbering(&irlist, data);

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.front());
    ASSERT_EQ(first->var.version, 1);

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.back());
    ASSERT_EQ(last->var.version, 2);
}

TEST(value_numbering, branching) {
    auto irlist = lower(R"EOF(
        x = 7
        x = 8
        if true
            x = 9
        else
            x = 10
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::block_walker(
        &irlist, {[&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); }});

    const auto & first = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.front());
    ASSERT_EQ(first->var.version, 1);

    const auto & last = std::get<std::unique_ptr<MIR::Number>>(irlist.instructions.back());
    ASSERT_EQ(last->var.version, 2);

    const auto & bb1 = get_con(irlist.next)->if_true;
    const auto & bb1_val = std::get<std::unique_ptr<MIR::Number>>(bb1->instructions.front());
    ASSERT_EQ(bb1_val->var.version, 3);

    const auto & bb2 = get_con(irlist.next)->if_false;
    const auto & bb2_val = std::get<std::unique_ptr<MIR::Number>>(bb2->instructions.front());
    ASSERT_EQ(bb2_val->var.version, 4);
}

TEST(insert_phi, simple) {
    auto irlist = lower(R"EOF(
        if true
            x = 9
        else
            x = 10
        endif
        )EOF");
    std::unordered_map<std::string, uint32_t> data{};
    MIR::Passes::block_walker(
        &irlist, {
                     [&](MIR::BasicBlock * b) { return MIR::Passes::value_numbering(b, data); },
                     MIR::Passes::insert_phis,
                 });

    const auto & fin = get_bb(get_con(irlist.next)->if_false->next);
    ASSERT_EQ(fin->instructions.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::unique_ptr<MIR::Phi>>(fin->instructions.front()));

    const auto & phi = std::get<std::unique_ptr<MIR::Phi>>(fin->instructions.front());
    ASSERT_EQ(phi->left, 0);
    ASSERT_EQ(phi->right, 1);
    ASSERT_EQ(phi->var.name, "x");
    ASSERT_EQ(phi->var.version, 0);
}
