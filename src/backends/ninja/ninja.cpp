// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Main ninja backend entry point.
 */

#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <variant>
#include <vector>

#include "entry.hpp"
#include "exceptions.hpp"
#include "toolchains/compiler.hpp"

namespace fs = std::filesystem;

namespace Backends::Ninja {

namespace {

void write_compiler_rule(const std::string & lang,
                         const std::unique_ptr<MIR::Toolchain::Compiler::Compiler> & c,
                         std::ofstream & out) {

    // TODO: build or host correctly
    out << "rule " << lang << "_compiler_for_"
        << "build" << std::endl;

    // Write the command
    // TODO: write the depfile stuff
    out << "  command =";
    for (const auto & c : c->command) {
        out << " " << c;
    }
    out << " ${ARGS}";
    for (const auto & c : c->output_command("${out}")) {
        out << " " << c;
    }
    for (const auto & c : c->compile_only_command()) {
        out << " " << c;
    }
    out << " ${in}" << std::endl;

    // Write the description
    out << "  description = Compiling " << c->language() << " object ${out}" << std::endl
        << std::endl;
}

void write_archiver_rule(const std::string & lang,
                         const std::unique_ptr<MIR::Toolchain::Archiver::Archiver> & c,
                         std::ofstream & out) {

    // TODO: build or host correctly
    out << "rule " << lang << "_archiver_for_"
        << "build" << std::endl;

    // Write the command
    // TODO: write the depfile stuff
    out << "  command =";
    out << "rm -f ${out} &&";
    for (const auto & c : c->command()) {
        out << " " << c;
    }
    out << " ${ARGS} ${out} ${in}\n";

    // Write the description
    out << "  description = Linking Static target ${out}\n" << std::endl;
}

void write_linker_rule(const std::string & lang,
                       const std::unique_ptr<MIR::Toolchain::Linker::Linker> & c,
                       std::ofstream & out) {

    // TODO: build or host correctly
    out << "rule " << lang << "_linker_for_"
        << "build" << std::endl;

    // Write the command
    // TODO: write the depfile stuff
    out << "  command =";
    for (const auto & c : c->command()) {
        out << " " << c;
    }
    out << " ${ARGS}";
    for (const auto & c : c->output_command("${out}")) {
        out << " " << c;
    }
    out << " ${in} ${ARGS}" << std::endl;

    // Write the description
    out << "  description = Linking target ${out}" << std::endl << std::endl;
}

std::string escape(const std::string & str) {
    std::string new_s{};
    for (const auto & s : str) {
        if (s == ' ') {
            new_s.push_back('$');
            new_s.push_back(s);
        } else {
            new_s.push_back(s);
        }
    }
    return new_s;
}

enum class RuleType {
    COMPILE,
    ARCHIVE,
    LINK,
};

/**
 * A Ninja rule to be generated later
 */
class Rule {
  public:
    Rule(const std::vector<std::string> & in, const std::vector<std::string> & out,
         const RuleType & r, const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m)
        : input{in}, output{out}, type{r}, lang{l}, machine{m}, arguments{} {};
    Rule(const std::vector<std::string> & in, const std::string & out, const RuleType & r,
         const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m,
         const std::vector<std::string> & args)
        : input{in}, output{out}, type{r}, lang{l}, machine{m}, arguments{args} {};

    /// The input for this rule
    const std::vector<std::string> input;

    /// The output of this rule
    const std::vector<std::string> output;

    /// The type of rule this is
    const RuleType type;

    /// The language of this rule
    const MIR::Toolchain::Language lang;

    /// The machine of this rule
    const MIR::Machines::Machine machine;

    /// The arguments for this rule
    const std::vector<std::string> arguments;
};

void write_build_rule(const Rule & rule, std::ofstream & out) {
    // TODO: get the actual compiler/linker
    std::string rule_name;
    switch (rule.type) {
        case RuleType::COMPILE:
            rule_name = "cpp_compiler_for_build";
            break;
        case RuleType::LINK:
            rule_name = "cpp_linker_for_build";
            break;
        case RuleType::ARCHIVE: // TODO:
            rule_name = "cpp_archiver_for_build";
            break;
        default:
            throw std::exception{}; // should be unreachable
    }

    out << "build";
    for (const auto & o : rule.output) {
        out << " " << o;
    }
    out << ": " << rule_name;
    for (const auto & o : rule.input) {
        out << " " << o;
    }
    out << "\n";

    out << "  ARGS =";
    for (const auto & a : rule.arguments) {
        out << " " << a;
    }
    out << "\n" << std::endl;
}

template <typename T>
std::vector<Rule> target_rule(const T & e, const MIR::State::Persistant & pstate) {
    static_assert(std::is_base_of<MIR::Executable, T>::value ||
                      std::is_base_of<MIR::StaticLibrary, T>::value,
                  "Must be derived from a build target");

    std::vector<std::string> cpp_args{};
    if (e.arguments.find(MIR::Toolchain::Language::CPP) != e.arguments.end()) {
        const auto & tc = pstate.toolchains.at(MIR::Toolchain::Language::CPP);
        for (const auto & a : e.arguments.at(MIR::Toolchain::Language::CPP)) {
            const auto & args =
                tc.build()->compiler->specialize_argument(a, pstate.source_root, pstate.build_root);
            for (const auto & arg : args) {
                cpp_args.emplace_back(escape(arg));
            }
        }
    }

    std::vector<Rule> rules{};
    const auto & tc = pstate.toolchains.at(MIR::Toolchain::Language::CPP);

    // These are the same on each iteration
    const auto & always_args = tc.build()->compiler->always_args();

    // TODO: there's a keyword argument to control this
    auto lincs = tc.build()->compiler->specialize_argument(
        MIR::Arguments::Argument(e.subdir, MIR::Arguments::Type::INCLUDE), pstate.source_root,
        pstate.build_root);
    for (const auto & arg : lincs) {
        cpp_args.emplace_back(escape(arg));
    }

    std::vector<std::string> srcs{};
    for (const auto & f : e.sources) {
        // TODO: obj files are a per compiler thing, I think
        // TODO: get the proper language
        // TODO: actually set args to something
        // TODO: do something better for private dirs, we really need the subdir for this

        auto lang_args = cpp_args;
        lang_args.insert(lang_args.end(), always_args.begin(), always_args.end());

        rules.emplace_back(Rule{{escape(f.relative_to_build_dir())},
                                escape(fs::path{e.name + ".p"} / f.get_name()) + ".o",
                                RuleType::COMPILE,
                                MIR::Toolchain::Language::CPP,
                                MIR::Machines::Machine::BUILD,
                                lang_args});
    }

    std::vector<std::string> final_outs;
    for (const auto & r : rules) {
        final_outs.insert(final_outs.end(), r.output.begin(), r.output.end());
    }
    for (const auto & [_, l] : e.link_static) {
        final_outs.emplace_back(l->output());
    }

    std::string name;
    RuleType type;
    std::vector<std::string> link_args{};
    if constexpr (std::is_base_of<MIR::StaticLibrary, T>::value) {
        type = RuleType::ARCHIVE;
        // TODO: per platform?
        name = e.output();
        // TODO: need to combin with link_arguments from DSL
        link_args = tc.build()->archiver->always_args();
    } else {
        type = RuleType::LINK;
        name = e.output();
        link_args = tc.build()->linker->always_args();
    }

    // TODO: linker/archiver always_args

    rules.emplace_back(Rule{
        final_outs,
        name,
        type,
        MIR::Toolchain::Language::CPP,
        MIR::Machines::Machine::BUILD,
        link_args,
    });

    return rules;
}

std::vector<Rule> mir_to_rules(const MIR::BasicBlock * const block,
                               const MIR::State::Persistant & pstate) {
    // A list of all rules
    std::vector<Rule> rules{};

    // A mapping of named targets to their rules.
    std::unordered_map<std::string, const Rule * const> rule_map{};

    for (const auto & i : block->instructions) {
        if (std::holds_alternative<std::shared_ptr<MIR::Executable>>(i)) {
            auto r = target_rule(*std::get<std::shared_ptr<MIR::Executable>>(i), pstate);
            std::move(r.begin(), r.end(), std::back_inserter(rules));
            const Rule * const named_rule = &rules.back();
            rule_map.emplace(named_rule->output, named_rule);
        } else if (std::holds_alternative<std::shared_ptr<MIR::StaticLibrary>>(i)) {
            auto r = target_rule(*std::get<std::shared_ptr<MIR::StaticLibrary>>(i), pstate);
            std::move(r.begin(), r.end(), std::back_inserter(rules));
            const Rule * const named_rule = &rules.back();
            rule_map.emplace(named_rule->output, named_rule);
        }
    }

    return rules;
}

} // namespace

void generate(const MIR::BasicBlock * const block, const MIR::State::Persistant & pstate) {
    if (!fs::exists(pstate.build_root)) {
        std::error_code ec{};
        fs::create_directory(pstate.build_root, ec);
        // XXX: actually check the error codes
        if (ec) {
            throw Util::Exceptions::MesonException{"Could not create build directory"};
        }

        ec.clear();

        // TODO: are these permissions really a good idea?
        fs::permissions(pstate.build_root,
                        fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all, ec);
        // XXX: actually check the error codes
        if (ec) {
            throw Util::Exceptions::MesonException{"Could not create build directory"};
        }
    }

    std::ofstream out{};
    out.open(pstate.build_root / "build.ninja", std::ios::out | std::ios::trunc);
    out << "# This is a build file for the project \"" << pstate.name << "\"." << std::endl
        << "# It is autogenerated by the Meson++ build system." << std::endl
        << "# Do not edit by hand." << std::endl
        << std::endl
        << "ninja_required_version = 1.8.2" << std::endl
        << std::endl;

    out << "# Compilation rules" << std::endl << std::endl;

    for (const auto & [l, tc] : pstate.toolchains) {
        const auto & lstr = MIR::Toolchain::to_string(l);
        // TODO: should also have a _for_host
        write_compiler_rule(lstr, tc.build()->compiler, out);
    }

    out << "# Static Linking rules" << std::endl << std::endl;

    for (const auto & [l, tc] : pstate.toolchains) {
        const auto & lstr = MIR::Toolchain::to_string(l);
        // TODO: should also have a _for_host
        write_archiver_rule(lstr, tc.build()->archiver, out);
    }

    out << "# Dynamic Linking rules" << std::endl << std::endl;

    for (const auto & [l, tc] : pstate.toolchains) {
        const auto & lstr = MIR::Toolchain::to_string(l);
        // TODO: should also have a _for_host
        write_linker_rule(lstr, tc.build()->linker, out);
    }

    out << "# Phony build target, always out of date\n\n"
        << "build PHONY: phony\n\n";
    out << "# Build rules for targets\n\n";

    const auto & rules = mir_to_rules(block, pstate);
    for (const auto & r : rules) {
        write_build_rule(r, out);
    }

    out.flush();
    out.close();
}

} // namespace Backends::Ninja
