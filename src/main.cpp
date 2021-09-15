// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/**
 * Main Meson++ entrypoint
 */

#include <filesystem>
#include <iostream>

#include "driver.hpp"
#include "log.hpp"
#include "options.hpp"
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

    std::cout << block->as_string() << std::endl;

    return 0;
};

int main(int argc, char * argv[]) {
    const auto opts = Options::parse_opts(argc, argv);

    int ret = 1;

    switch (opts.verb) {
        case Options::Verb::CONFIGURE:
            ret = configure(opts.config);
            break;
    };

    return ret;
}
