// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <iostream>

#include "getopt.h" // XXX: This is probably not permanent

#include "exceptions.hpp"
#include "options.hpp"

namespace Options {

namespace {

// clang-format off
const std::string usage =
R"EOF(Usage:
    meson++ <verb> [verb_options]

Verbs:
    Configure:
        Usage:
            meson++ configure <builddir> [options]

        setup a new build directory, or change the configuration of a build directory

        Options:
            -h, --help
                Display this message and exit.

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

    static const char * const short_opts = "h";
    static const option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {NULL},
    };

    int c;
    while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL))) {
        switch (c) {
            case 'h':
            default:
                std::cout << usage << std::endl;
                exit(0);
        }
    }

    int i = optind;
    if (i >= argc) {
        std::cerr << "missing required positional argument to 'meson++ configure', <builddir>"
                  << std::endl;
        std::cout << usage << std::endl;
        exit(1);
    }
    conf.builddir = argv[i++];

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
