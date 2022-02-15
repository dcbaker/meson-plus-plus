// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

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
            if (std::holds_alternative<std::unique_ptr<MIR::Message>>(i)) {
                const auto & m = std::get<std::unique_ptr<MIR::Message>>(i);
                if (m->level == level) {
                    std::cout << Util::Log::bold(" *  ") << m->message << std::endl;
                }
                if (m->level == MIR::MessageLevel::ERROR) {
                    errors = true;
                }
            }
        }
    }
    return errors;
}

} // namespace

static int configure(const Options::ConfigureOptions & opts) {
    std::cout << Util::Log::bold("The Meson++ build system") << std::endl
              << "Version: " << version::VERSION << std::endl
              << "Source dir: " << Util::Log::bold(fs::absolute(opts.sourcedir)) << std::endl
              << "Build dir: " << Util::Log::bold(fs::absolute(opts.builddir)) << std::endl;

    // Parse the source into a an AST
    Frontend::Driver drv{};
    auto block = drv.parse(opts.sourcedir / "meson.build");

    MIR::State::Persistant pstate{opts.sourcedir, opts.builddir};

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

int main(int argc, char * argv[]) {
    const auto opts = Options::parse_opts(argc, argv);

    int ret = 1;

    try {
        switch (opts.verb) {
            case Options::Verb::CONFIGURE:
                ret = configure(opts.config);
                break;
        };
    } catch (Util::Exceptions::MesonException & e) {
        std::cerr << "Meson++ error: " << e.what() << std::endl;
    } catch (std::exception & e) {
        std::cerr << "Uncaught general exceptions: " << e.what() << std::endl;
    }

    return ret;
}
