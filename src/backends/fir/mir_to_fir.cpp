// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include "fir.hpp"
#include "meson/arguments.hpp"

#include <vector>

namespace Backends::FIR {

namespace {

/// @brief FIR backend state
struct State {
    /// @brief Combined global and project arguments for each project
    /// TODO: currently stored as a vector because there are no projects
    /// implemented, but eventually needs to be stored as a map?
    MIR::ArgMap arguments;
};

template <typename T>
std::vector<Target> target_rule(const std::shared_ptr<T> & e, const MIR::State::Persistant & pstate,
                                const State & state) {
    static_assert(std::is_base_of<MIR::Executable, T>::value ||
                      std::is_base_of<MIR::StaticLibrary, T>::value,
                  "Must be derived from a build target");

    const auto & tc = pstate.toolchains.at(MIR::Toolchain::Language::CPP);

    std::vector<std::string> cpp_args{};
    if (state.arguments.find(MIR::Toolchain::Language::CPP) != state.arguments.end()) {
        for (const auto & a : state.arguments.at(MIR::Toolchain::Language::CPP)) {
            auto args =
                tc.build()->compiler->specialize_argument(a, pstate.source_root, pstate.build_root);
            std::move(args.begin(), args.end(), std::back_inserter(cpp_args));
        }
    }
    if (e->arguments.find(MIR::Toolchain::Language::CPP) != e->arguments.end()) {
        for (const auto & a : e->arguments.at(MIR::Toolchain::Language::CPP)) {
            auto args =
                tc.build()->compiler->specialize_argument(a, pstate.source_root, pstate.build_root);
            std::move(args.begin(), args.end(), std::back_inserter(cpp_args));
        }
    }

    std::vector<Target> rules{};

    // These are the same on each iteration
    const auto & always_args = tc.build()->compiler->always_args();

    // TODO: there's a keyword argument to control this
    auto lincs = tc.build()->compiler->specialize_argument(
        MIR::Arguments::Argument(e->subdir, MIR::Arguments::Type::INCLUDE), pstate.source_root,
        pstate.build_root);
    std::move(lincs.begin(), lincs.end(), std::back_inserter(cpp_args));

    std::vector<std::string> order_deps{};
    for (const auto & f : e->sources) {
        if (std::holds_alternative<MIR::CustomTargetPtr>(f)) {
            const auto & t = std::get<MIR::CustomTargetPtr>(f);
            for (const auto & ff : t->outputs) {
                if (tc.build()->compiler->supports_file(ff->get_name()) ==
                    MIR::Toolchain::Compiler::CanCompileType::DEPENDS) {
                    order_deps.emplace_back(ff->relative_to_build_dir());
                }
            }
        }
    }

    std::vector<std::string> srcs{};
    for (const auto & f : e->sources) {
        // TODO: obj files are a per compiler thing, I think
        // TODO: get the proper language
        // TODO: actually set args to something
        // TODO: do something better for private dirs, we really need the subdir for this

        auto lang_args = cpp_args;
        lang_args.insert(lang_args.end(), always_args.begin(), always_args.end());

        // FIXME: without depfile support, we can't really treat order only deps
        // correctly, and instead we have to treat them as full deps for correct
        // behavior. This should be fixed.
        if (std::holds_alternative<MIR::FilePtr>(f)) {
            const auto & ff = std::get<MIR::FilePtr>(f);
            if (tc.build()->compiler->supports_file(ff->get_name()) ==
                MIR::Toolchain::Compiler::CanCompileType::SOURCE) {
                rules.emplace_back(
                    Target{{ff->relative_to_build_dir()},
                           std::string{fs::path{e->name + ".p"} / ff->get_name()} + ".o",
                           TargetType::COMPILE,
                           MIR::Toolchain::Language::CPP,
                           MIR::Machines::Machine::BUILD,
                           lang_args,
                           {},
                           order_deps});
            }
        } else {
            const auto & t = std::get<MIR::CustomTargetPtr>(f);
            for (const auto & ff : t->outputs) {
                if (tc.build()->compiler->supports_file(ff->get_name()) ==
                    MIR::Toolchain::Compiler::CanCompileType::SOURCE) {
                    rules.emplace_back(
                        Target{{ff->relative_to_build_dir()},
                               std::string{fs::path{e->name + ".p"} / ff->get_name()} + ".o",
                               TargetType::COMPILE,
                               MIR::Toolchain::Language::CPP,
                               MIR::Machines::Machine::BUILD,
                               lang_args,
                               {ff->relative_to_build_dir()},
                               order_deps});
                }
            }
        }
    }

    std::vector<std::string> final_outs;
    for (const auto & r : rules) {
        final_outs.insert(final_outs.end(), r.output.begin(), r.output.end());
    }
    for (const auto & [_, l] : e->link_static) {
        final_outs.emplace_back(l->output());
    }

    std::string name;
    TargetType type;
    std::vector<std::string> link_args{};
    if constexpr (std::is_base_of<MIR::StaticLibrary, T>::value) {
        type = TargetType::ARCHIVE;
        // TODO: per platform?
        name = e->output();
        // TODO: need to combin with link_arguments from DSL
        link_args = tc.build()->archiver->always_args();
    } else {
        type = TargetType::LINK;
        name = e->output();
        link_args = tc.build()->linker->always_args();
    }

    // TODO: linker/archiver always_args

    rules.emplace_back(Target{
        final_outs,
        name,
        type,
        MIR::Toolchain::Language::CPP,
        MIR::Machines::Machine::BUILD,
        link_args,
    });

    return rules;
}

template <>
std::vector<Target> target_rule<MIR::CustomTarget>(const MIR::CustomTargetPtr & e,
                                                   const MIR::State::Persistant & pstate,
                                                   const State & state) {
    std::vector<std::string> outs{};
    for (const auto & o : e->outputs) {
        outs.emplace_back(o->relative_to_build_dir());
    }

    std::vector<std::string> ins{};
    for (const auto & i : e->inputs) {
        if (std::holds_alternative<MIR::FilePtr>(i)) {
            const auto f = std::get<MIR::FilePtr>(i);
            ins.emplace_back(f->relative_to_build_dir());
        } else {
            const auto c = std::get<MIR::CustomTargetPtr>(i);
            for (const auto & f : c->outputs) {
                ins.emplace_back(f->relative_to_build_dir());
            }
        }
    }

    std::vector<std::string> depends;
    for (auto && d : e->depends) {
        depends.emplace_back(d->relative_to_build_dir());
    }

    return {Target{ins, outs, TargetType::CUSTOM, e->command, depends, e->depfile}};
}

struct TestVisitor {
    TestVisitor() = default;

    fs::path operator()(const MIR::FilePtr callable) { return callable->relative_to_build_dir(); }
    fs::path operator()(const MIR::ExecutablePtr callable) {
        return callable->subdir / callable->output();
    };
    fs::path operator()(const MIR::ProgramPtr callable) { return callable->path; };
};

struct ToStringVisitor {
    std::string operator()(const MIR::FilePtr f) { return f->relative_to_build_dir(); }
    std::string operator()(const MIR::StringPtr f) { return f->value; }
    // std::string operator()(const std::monostate &) { return ""; }
};

Common::Test target_test(const MIR::TestPtr t, const MIR::State::Persistant & pstate) {
    TestVisitor visitor{};
    ToStringVisitor str_visitor{};
    fs::path && output = std::visit(visitor, t->executable);
    std::vector<std::string> arguments;
    for (auto && a : t->arguments) {
        if (std::string arg = std::visit(str_visitor, a); !arg.empty()) {
            arguments.emplace_back(arg);
        }
    }
    return Common::Test{t->name, output, arguments, t->should_fail};
}

} // namespace

std::tuple<std::vector<Target>, std::vector<Common::Test>>
mir_to_fir(const MIR::CFGNode & block, const MIR::State::Persistant & pstate) {
    State state{};
    // Process the instructions that alter state
    for (const auto & i : block.block->instructions) {
        if (std::holds_alternative<MIR::AddArgumentsPtr>(i)) {
            // TODO: actually handle global vs per-project
            const auto & argmap = std::get<MIR::AddArgumentsPtr>(i);
            for (auto && [lang, args] : argmap->arguments) {
                state.arguments[lang].insert(state.arguments[lang].begin(), args.begin(),
                                             args.end());
            }
        }
    }

    // A list of all rules
    std::vector<Target> rules{};
    std::vector<Common::Test> tests{};

    for (const auto & i : block.block->instructions) {
        if (std::holds_alternative<MIR::ExecutablePtr>(i)) {
            auto r = target_rule(std::get<MIR::ExecutablePtr>(i), pstate, state);
            std::move(r.begin(), r.end(), std::back_inserter(rules));
        } else if (std::holds_alternative<MIR::StaticLibraryPtr>(i)) {
            auto r = target_rule(std::get<MIR::StaticLibraryPtr>(i), pstate, state);
            std::move(r.begin(), r.end(), std::back_inserter(rules));
        } else if (std::holds_alternative<MIR::CustomTargetPtr>(i)) {
            auto r = target_rule(std::get<MIR::CustomTargetPtr>(i), pstate, state);
            std::move(r.begin(), r.end(), std::back_inserter(rules));
        } else if (std::holds_alternative<MIR::TestPtr>(i)) {
            tests.emplace_back(target_test(std::get<MIR::TestPtr>(i), pstate));
        }
    }

    return std::make_tuple(rules, tests);
}

} // namespace Backends::FIR
