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
    MIR::Passes::lower_project(&irlist, pstate);
    MIR::lower(&irlist, pstate);

    Backends::Ninja::generate(&irlist, pstate);

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

        return ret;
    } catch (Util::Exceptions::MesonException & e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
