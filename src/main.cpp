// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2026 Intel Corporation

/**
 * Main Meson++ entrypoint
 */

#include "driver.hpp"
#include "exceptions.hpp"
#include "log.hpp"
#include "options.hpp"
#include "tools/compile.hpp"
#include "tools/test.hpp"
#include "tools/vcs_tag.hpp"
#include "version.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace {

int configure(const Options::ConfigureOptions & opts) {
    std::cout << Util::Log::bold("The Meson++ build system") << std::endl
              << "Version: " << version::VERSION << std::endl
              << "Source dir: " << Util::Log::bold(fs::absolute(opts.sourcedir)) << std::endl
              << "Build dir: " << Util::Log::bold(fs::absolute(opts.builddir)) << std::endl;

    // Parse the source into a an AST
    Frontend::Driver drv{};
    auto block = drv.parse(opts.sourcedir / "meson.build");

    return 0;
};

struct OptionHandler {
    int operator()(const Options::ConfigureOptions & opts) { return configure(opts); }
    int operator()(const Options::VCSTagOptions & opts) {
        return Tools::generate_vcs_tag(opts.infile, opts.outfile, opts.version, opts.replacement,
                                       opts.source_dir, opts.depfile);
    }
    int operator()(const Options::CompileOptions & opts) { return Tools::compile(opts.infile); }
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
