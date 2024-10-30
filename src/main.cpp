// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Main Meson++ entrypoint
 */

#include <filesystem>
#include <iostream>

#include "ast_to_mir.hpp"
#include "backends/ninja/entry.hpp"
#include "driver.hpp"
#include "exceptions.hpp"
#include "log.hpp"
#include "lower.hpp"
#include "options.hpp"
#include "state/state.hpp"
#include "tools/test.hpp"
#include "tools/vcs_tag.hpp"
#include "version.hpp"

namespace fs = std::filesystem;

namespace {

bool emit_messages(MIR::BasicBlock & block) {
    static std::vector<MIR::MessageLevel> levels{MIR::MessageLevel::MESSAGE,
                                                 MIR::MessageLevel::WARN, MIR::MessageLevel::ERROR};
    bool errors = false;

    for (const auto & level : levels) {
        if (level == MIR::MessageLevel::MESSAGE) {
            std::cout << Util::Log::bold("User Messages:") << std::endl;
        } else if (level == MIR::MessageLevel::WARN) {
            std::cout << Util::Log::yellow("Warnings:") << std::endl;
        } else if (level == MIR::MessageLevel::ERROR) {
            std::cout << Util::Log::red("Errors:") << std::endl;
        } else if (level == MIR::MessageLevel::DEBUG) {
            std::cout << Util::Log::bold("Debug information:") << std::endl;
        }
        for (const auto & i : block.instructions) {
            if (std::holds_alternative<MIR::Message>(*i.obj_ptr)) {
                const auto & m = std::get<MIR::Message>(*i.obj_ptr);
                if (m.level == level) {
                    std::cout << Util::Log::bold(" *  ") << m.message << std::endl;
                }
                if (m.level == MIR::MessageLevel::ERROR) {
                    errors = true;
                }
            }
        }
    }
    return errors;
}

int configure(const Options::ConfigureOptions & opts) {
    std::cout << Util::Log::bold("The Meson++ build system") << std::endl
              << "Version: " << version::VERSION << std::endl
              << "Source dir: " << Util::Log::bold(fs::absolute(opts.sourcedir)) << std::endl
              << "Build dir: " << Util::Log::bold(fs::absolute(opts.builddir)) << std::endl;

    // Parse the source into a an AST
    Frontend::Driver drv{};
    auto block = drv.parse(opts.sourcedir / "meson.build");

    MIR::State::Persistant pstate{opts.sourcedir, opts.builddir, opts.program};

    // Create IR from the AST, then run our lowering passes on it
    auto irlist = MIR::lower_ast(block, pstate);
    MIR::Passes::lower_project(irlist, pstate);
    MIR::lower(irlist, pstate);

    const bool errors = emit_messages(irlist);
    if (errors) {
        throw Util::Exceptions::MesonException("Configure failed with errors.");
    }

    Backends::Ninja::generate(irlist, pstate);

    return 0;
};

int test(const Options::TestOptions & opts) {
    auto && path = opts.builddir / "tests.serialized";
    if (!fs::exists(path)) {
        std::cout << "No tests defined" << std::endl;
        return 0;
    }

    auto && tests = Backends::Common::load_tests(path);
    return Tools::run_tests(tests, fs::absolute(opts.builddir));
}

struct OptionHandler {
    int operator()(const Options::ConfigureOptions & opts) { return configure(opts); }
    int operator()(const Options::TestOptions & opts) { return test(opts); }
    int operator()(const Options::VCSTagOptions & opts) {
        return Tools::generate_vcs_tag(opts.infile, opts.outfile, opts.version, opts.replacement,
                                       opts.source_dir, opts.depfile);
    }
};

} // namespace

int main(int argc, char * argv[]) {
    auto && opts = Options::parse_opts(argc, argv);

    try {
        return std::visit(OptionHandler{}, opts);
    } catch (Util::Exceptions::MesonException & e) {
        std::cerr << "Meson++ error: " << e.what() << std::endl;
    } catch (std::exception & e) {
        std::cerr << "Uncaught general exceptions: " << e.what() << std::endl;
    }
}
