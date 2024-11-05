// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Main ninja backend entry point.
 */

#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <variant>
#include <vector>

#include "common/backend.hpp"
#include "entry.hpp"
#include "exceptions.hpp"
#include "fir/fir.hpp"
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
    for (const auto & a : c->generate_depfile("${out}", "$DEPFILE")) {
        out << " " << a;
    }
    for (const auto & a : c->output_command("${out}")) {
        out << " " << a;
    }
    for (const auto & a : c->compile_only_command()) {
        out << " " << a;
    }
    out << " ${in}" << std::endl;

    // TODO: control support for this
    // TODO: MSVC style deps
    // FIXME: why does meson write this out with two different vlues?
    out << "  deps = gcc" << std::endl;
    out << "  depfile = $DEPFILE_UNQUOTED" << std::endl;

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

std::string escape(const std::string & str, const bool & quote = false) {
    std::string new_s{};
    bool needs_quote = false;
    for (const auto & s : str) {
        if (s == ' ' || s == '$') {
            new_s.push_back('$');
            new_s.push_back(s);
            needs_quote = true;
        } else {
            new_s.push_back(s);
        }
    }
    if (quote && needs_quote) {
        new_s = "'" + new_s + "'";
    }
    return new_s;
}

void write_build_rule(const FIR::Target & rule, std::ofstream & out) {
    // TODO: get the actual compiler/linker
    std::string rule_name;
    switch (rule.type) {
        case FIR::TargetType::COMPILE:
            rule_name = "cpp_compiler_for_build";
            break;
        case FIR::TargetType::LINK:
            rule_name = "cpp_linker_for_build";
            break;
        case FIR::TargetType::ARCHIVE:
            rule_name = "cpp_archiver_for_build";
            break;
        case FIR::TargetType::CUSTOM:
            if (rule.depfile) {
                rule_name = "custom_command_dep";
            } else {
                rule_name = "custom_command";
            }
            break;
        default:
            throw std::exception{}; // should be unreachable
    }

    // writes out the main build line in the form:
    //`build {outputs}: {rule} {inputs} | {deps} || {order deps}`
    out << "build";
    // Write outputs
    for (const auto & o : rule.output) {
        out << " " << escape(o);
    }
    // rule name
    out << ": " << rule_name;
    // inputs
    for (const auto & o : rule.input) {
        out << " " << escape(o);
    }

    if (!rule.deps.empty()) {
        out << " |";
        for (const auto & d : rule.deps) {
            out << " " << escape(d);
        }
    }

    if (!rule.order_deps.empty()) {
        out << " ||";
        for (const auto & d : rule.order_deps) {
            out << " " << escape(d);
        }
    }

    out << "\n";

    // Write out the arguments to be used by the rule
    out << "  ARGS =";
    for (const auto & a : rule.arguments) {
        out << " " << escape(a, true);
    }
    out << "\n";

    // Write the depfile
    // TODO: better control of when to do this
    if (rule.type == FIR::TargetType::COMPILE) {
        out << "  DEPFILE = " << escape(rule.output[0]) << ".d" << std::endl;
        out << "  DEPFILE_UNQUOTED = " << rule.output[0] << ".d" << std::endl;
    }

    if (rule.type == FIR::TargetType::CUSTOM) {
        out << "  DESCRIPTION = " << escape("generating ") << escape(rule.output[0])
            << escape(" with ") << escape(rule.arguments[0]) << "\n";
        if (rule.depfile) {
            out << "  DEPFILE_UNQUOTED = " << rule.depfile.value() << "\n";
        }
    }
    out << std::endl;
}

} // namespace

void generate(const MIR::CFGNode & block, const MIR::State::Persistant & pstate) {
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

    out << "rule custom_command\n"
        << "  command = $ARGS\n"
        << "  description = $DESCRIPTION\n"
        << "  restat = 1\n\n";

    out << "rule custom_command_dep\n"
        << "  command = $ARGS\n"
        << "  description = $DESCRIPTION\n"
        << "  deps = gcc\n"
        << "  depfile = $DEPFILE_UNQUOTED\n"
        << "  restat = 1\n\n";

    out << "# Phony build target, always out of date\n\n"
        << "build PHONY: phony\n\n"
        << "# Build rules for targets\n\n";

    auto && [rules, tests] = FIR::mir_to_fir(block, pstate);
    for (const auto & r : rules) {
        write_build_rule(r, out);
    }

    out.flush();
    out.close();

    if (!tests.empty()) {
        Common::serialize_tests(tests, pstate.build_root / "tests.serialized");
    }
}

} // namespace Backends::Ninja
