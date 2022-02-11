// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <iostream>

#include "getopt.h" // XXX: This is probably not permanent

#include "exceptions.hpp"
#include "options.hpp"
#include "version.hpp"

namespace Options {

namespace {

// clang-format off
const std::string usage =
"Meson++ Version " + version::VERSION +
R"EOF(

Usage:
    meson++ <verb> [verb_options]

Description:
    Meson++ is an implementation of the Meson build system, written in C++

Verbs:
    Configure:
        Usage:
            meson++ configure <builddir> [options]

        setup a new build directory, or change the configuration of a build directory

        Options:
            -h, --help
                Display this message and exit.
            -s, --source-dir
                The source directory to configure, defaults to '.'
            -D, --define
                Set a Meson built-in or project option

)EOF";
// clang-format on

Verb get_verb(int & argc, const char * const argv[]) {
    if (argc > 1) {
        const std::string v{argv[1]};

        if (v == "configure") {
            return Verb::CONFIGURE;
        }

        std::cerr << "Unknown action:" << v << std::endl;
    }

    std::cout << usage << std::endl;
    exit(1);
}

ConfigureOptions get_config_options(int argc, char * argv[]) {
    ConfigureOptions conf{};

    static const char * const short_opts = "hs:D:";
    static const option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"source-dir", required_argument, NULL, 's'},
        {"define", required_argument, NULL, 'D'},
        {NULL},
    };

    // Initialize the sourcedir
    conf.sourcedir = fs::path{"."};

    int c;
    while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (c) {
            case 's':
                conf.sourcedir = fs::path{optarg};
                break;
            case 'D': {
                const std::string d{optarg};
                const auto n = d.find('=');
                if (n == std::string::npos) {
                    std::cerr << "define options must be in the for `-Dopt=value` or `--define "
                                 "opt=value`. Option \""
                              << d << "\" does not have an \"=\"." << std::endl;
                    exit(1);
                }
                auto opt = d.substr(0, n);
                auto value = d.substr(n + 1, d.size());

                conf.options[opt] = value;
                break;
            }
            case 'h':
            default:
                std::cout << usage << std::endl;
                exit(0);
        }
    }

    // ++ here to pass the verb
    int i = ++optind;
    if (i >= argc) {
        std::cerr << "missing required positional argument to 'meson++ configure': <builddir>"
                  << std::endl;
        std::cout << usage << std::endl;
        exit(1);
    }
    conf.builddir = fs::path{argv[i++]};
    if (i < argc) {
        // TODO: better error message
        std::cerr << "Got extra arguments." << std::endl;
        std::cout << usage << std::endl;
        exit(1);
    }

    return conf;
}

} // namespace

Options parse_opts(int argc, char * argv[]) {
    if (argc < 1) {
        std::cerr << "Not enough arguments" << std::endl;
        std::cout << usage << std::endl;
    }
    // First we want to find which verb we're parsing
    Options opts{};
    opts.verb = get_verb(argc, argv);

    switch (opts.verb) {
        case Verb::CONFIGURE:
            opts.config = get_config_options(argc, argv);
    }

    return opts;
}

} // namespace Options
