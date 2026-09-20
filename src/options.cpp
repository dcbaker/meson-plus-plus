// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2026 Intel Corporation

#include "options.hpp"
#include "exceptions.hpp"

#include <CLI/CLI.hpp>

#include <iostream>

namespace Options {

OptionV parse_opts(int argc, char * argv[]) {
    CLI::App app{};
    app.require_subcommand(1);

    CompileOptions comp_opts{};
    CLI::App * compile_app = app.add_subcommand("compile", "serialize Meson build definitions");
    compile_app->add_option("source", comp_opts.infile, "The meson.build root file to compile")
        ->required();

    VCSTagOptions vctag_opts{};
    CLI::App * vcstag_app = app.add_subcommand("vcs_tag");
    vcstag_app->add_option("infile", vctag_opts.infile)->required();
    vcstag_app->add_option("outfile", vctag_opts.outfile)->required();
    vcstag_app->add_option("version", vctag_opts.version)->required();
    vcstag_app->add_option("replacement", vctag_opts.replacement)->required();
    vcstag_app->add_option("source_dir", vctag_opts.source_dir)->required();
    vcstag_app->add_option("depfile", vctag_opts.depfile)->required();

    ConfigureOptions conf_opts{};
    CLI::App * conf_app = app.add_subcommand("configure");

    conf_app
        ->add_option("build-dir", conf_opts.builddir,
                     "The directory in to place build files are located")
        ->required();

    conf_app
        ->add_option("source-dir", conf_opts.sourcedir,
                     "The directory in which the sources are located")
        ->default_val(".");

    conf_app->add_option(
        "-D",
        [&conf_opts](CLI::results_t res) {
            const std::string & v = res.at(0);
            auto && n = v.find('=');
            if (n == v.npos) {
                return false;
            }
            conf_opts.options.emplace(v.substr(0, n), v.substr(n + 1, v.size()));
            return true;
        },
        "Set configuration options");

    try {
        app.parse(argc, argv);
    } catch (CLI::ParseError & e) {
        exit(app.exit(e));
    }

    if (conf_app->parsed()) {
        return conf_opts;
    }
    if (compile_app->parsed()) {
        return comp_opts;
    }
    if (vcstag_app->parsed()) {
        return vctag_opts;
    }

    throw std::runtime_error{"Should be unreachable"};
}

} // namespace Options
