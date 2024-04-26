// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "fir.hpp"

namespace Backends::FIR {

namespace {

template <typename T>
std::vector<Target> target_rule(const T & e, const MIR::State::Persistant & pstate) {
    static_assert(std::is_base_of<MIR::Executable, T>::value ||
                      std::is_base_of<MIR::StaticLibrary, T>::value,
                  "Must be derived from a build target");

    std::vector<std::string> cpp_args{};
    if (e.arguments.find(MIR::Toolchain::Language::CPP) != e.arguments.end()) {
        const auto & tc = pstate.toolchains.at(MIR::Toolchain::Language::CPP);
        for (const auto & a : e.arguments.at(MIR::Toolchain::Language::CPP)) {
            const auto & args =
                tc.build()->compiler->specialize_argument(a, pstate.source_root, pstate.build_root);
            std::copy(args.begin(), args.end(), std::back_inserter(cpp_args));
        }
    }

    std::vector<Target> rules{};
    const auto & tc = pstate.toolchains.at(MIR::Toolchain::Language::CPP);

    // These are the same on each iteration
    const auto & always_args = tc.build()->compiler->always_args();

    // TODO: there's a keyword argument to control this
    auto lincs = tc.build()->compiler->specialize_argument(
        MIR::Arguments::Argument(e.subdir, MIR::Arguments::Type::INCLUDE), pstate.source_root,
        pstate.build_root);
    std::copy(lincs.begin(), lincs.end(), std::back_inserter(cpp_args));

    std::vector<std::string> order_deps{};
    for (const auto & f : e.sources) {
        if (std::holds_alternative<MIR::CustomTarget>(*f.obj_ptr)) {
            const auto & t = std::get<MIR::CustomTarget>(*f.obj_ptr);
            for (const auto & ff : t.outputs) {
                if (tc.build()->compiler->supports_file(ff.get_name()) ==
                    MIR::Toolchain::Compiler::CanCompileType::DEPENDS) {
                    order_deps.emplace_back(ff.relative_to_build_dir());
                }
            }
        }
    }

    std::vector<std::string> srcs{};
    for (const auto & f : e.sources) {
        // TODO: obj files are a per compiler thing, I think
        // TODO: get the proper language
        // TODO: actually set args to something
        // TODO: do something better for private dirs, we really need the subdir for this

        auto lang_args = cpp_args;
        lang_args.insert(lang_args.end(), always_args.begin(), always_args.end());

        // FIXME: without depfile support, we can't really treat order only deps
        // correctly, and instead we have to treat them as full deps for correct
        // behavior. This should be fixed.
        if (std::holds_alternative<MIR::File>(*f.obj_ptr)) {
            const auto & ff = std::get<MIR::File>(*f.obj_ptr);
            if (tc.build()->compiler->supports_file(ff.get_name()) ==
                MIR::Toolchain::Compiler::CanCompileType::SOURCE) {
                rules.emplace_back(
                    Target{{ff.relative_to_build_dir()},
                           std::string{fs::path{e.name + ".p"} / ff.get_name()} + ".o",
                           TargetType::COMPILE,
                           MIR::Toolchain::Language::CPP,
                           MIR::Machines::Machine::BUILD,
                           lang_args,
                           {},
                           order_deps});
            }
        } else {
            const auto & t = std::get<MIR::CustomTarget>(*f.obj_ptr);
            for (const auto & ff : t.outputs) {
                if (tc.build()->compiler->supports_file(ff.get_name()) ==
                    MIR::Toolchain::Compiler::CanCompileType::SOURCE) {
                    rules.emplace_back(
                        Target{{ff.relative_to_build_dir()},
                               std::string{fs::path{e.name + ".p"} / ff.get_name()} + ".o",
                               TargetType::COMPILE,
                               MIR::Toolchain::Language::CPP,
                               MIR::Machines::Machine::BUILD,
                               lang_args,
                               {ff.relative_to_build_dir()},
                               order_deps});
                }
            }
        }
    }

    std::vector<std::string> final_outs;
    for (const auto & r : rules) {
        final_outs.insert(final_outs.end(), r.output.begin(), r.output.end());
    }
    for (const auto & [_, l] : e.link_static) {
        final_outs.emplace_back(l.output());
    }

    std::string name;
    TargetType type;
    std::vector<std::string> link_args{};
    if constexpr (std::is_base_of<MIR::StaticLibrary, T>::value) {
        type = TargetType::ARCHIVE;
        // TODO: per platform?
        name = e.output();
        // TODO: need to combin with link_arguments from DSL
        link_args = tc.build()->archiver->always_args();
    } else {
        type = TargetType::LINK;
        name = e.output();
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
std::vector<Target> target_rule<MIR::CustomTarget>(const MIR::CustomTarget & e,
                                                   const MIR::State::Persistant & pstate) {
    std::vector<std::string> outs{};
    for (const auto & o : e.outputs) {
        outs.emplace_back(o.relative_to_build_dir());
    }

    std::vector<std::string> ins{};
    for (const auto & i : e.inputs) {
        if (std::holds_alternative<MIR::File>(*i.obj_ptr)) {
            const auto & f = std::get<MIR::File>(*i.obj_ptr);
            ins.emplace_back(f.relative_to_build_dir());
        } else {
            const auto & c = std::get<MIR::CustomTarget>(*i.obj_ptr);
            for (const auto & f : c.outputs) {
                ins.emplace_back(f.relative_to_build_dir());
            }
        }
    }

    return {Target{ins, outs, TargetType::CUSTOM, e.command}};
}

} // namespace

std::vector<Target> mir_to_fir(const MIR::BasicBlock & block,
                               const MIR::State::Persistant & pstate) {
    // A list of all rules
    std::vector<Target> rules{};

    for (const auto & i : block.instructions) {
        if (std::holds_alternative<MIR::Executable>(*i.obj_ptr)) {
            auto r = target_rule(std::get<MIR::Executable>(*i.obj_ptr), pstate);
            std::move(r.begin(), r.end(), std::back_inserter(rules));
        } else if (std::holds_alternative<MIR::StaticLibrary>(*i.obj_ptr)) {
            auto r = target_rule(std::get<MIR::StaticLibrary>(*i.obj_ptr), pstate);
            std::move(r.begin(), r.end(), std::back_inserter(rules));
        } else if (std::holds_alternative<MIR::CustomTarget>(*i.obj_ptr)) {
            auto r = target_rule(std::get<MIR::CustomTarget>(*i.obj_ptr), pstate);
            std::move(r.begin(), r.end(), std::back_inserter(rules));
        }
    }

    return rules;
}

} // namespace Backends::FIR
