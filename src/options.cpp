// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

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

Options:
    -h, --help
        Display this message and exit.

Verbs:
    Configure:
        Usage:
            meson++ configure <builddir> [options]

        setup a new build directory, or change the configuration of a build directory

        Options:
            -s, --source-dir
                The source directory to configure, defaults to '.'
            -D, --define
                Set a Meson built-in or project option

    Test:
        Usage:
            meson++ test <builddir> [options]

        Run tests on a new build directory.

    *:
        Any additional verbs that are not documented here are considered
        implementation details, and are subject to change at any time without
        warning.

)EOF";
// clang-format on

Verb get_verb(int & argc, const char * const argv[]) {
    if (argc > 1) {
        const std::string v{argv[1]};

        if (v == "configure") {
            return Verb::CONFIGURE;
        }
        if (v == "test") {
            return Verb::TEST;
        }
        if (v == "vcs_tag") {
            return Verb::VCS_TAG;
        }

        std::cerr << "Unknown action:" << v << std::endl;
    }

    std::cout << usage << std::endl;
    exit(1);
}

ConfigureOptions get_config_options(int argc, char * argv[]) {
    ConfigureOptions conf{
        .program = fs::absolute(argv[0]),
        .sourcedir = fs::current_path(),
    };

    static const char * const short_opts = "hs:D:";
    static const option long_opts[] = {
        {"help", no_argument, nullptr, 'h'},
        {"source-dir", required_argument, nullptr, 's'},
        {"define", required_argument, nullptr, 'D'},
        {nullptr},
    };

    int c;
    while ((c = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (c) {
            case 's':
                conf.sourcedir = fs::absolute(optarg);
                break;
            case 'D': {
                const std::string d{optarg};
                const auto n = d.find('=');
                if (n == std::string::npos) {
                    std::cerr << "define options must be in the for `-Dopt=value` or `--define "
                                 "opt=value`. Option \""
                              << d << R"(" does not have an "=".)" << std::endl;
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
    conf.builddir = fs::absolute(argv[i++]);
    if (i < argc) {
        // TODO: better error message
        std::cerr << "Got extra arguments." << std::endl;
        std::cout << usage << std::endl;
        exit(1);
    }

    return conf;
}

TestOptions get_test_options(int argc, char * argv[]) {
    TestOptions opts{};

    static const char * const short_opts = "hs:D:";
    static const option long_opts[] = {
        {"help", no_argument, nullptr, 'h'},
        {nullptr},
    };

    int c;
    while ((c = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (c) {
            case 'h':
            default:
                std::cout << usage << std::endl;
                exit(0);
        }
    }

    // ++ here to pass the verb
    int i = ++optind;
    if (i >= argc) {
        std::cerr << "missing required positional argument to 'meson++ test': <builddir>"
                  << std::endl;
        std::cout << usage << std::endl;
        exit(1);
    }
    opts.builddir = fs::path{argv[i++]};
    if (i < argc) {
        // TODO: better error message
        std::cerr << "Got extra arguments." << std::endl;
        std::cout << usage << std::endl;
        exit(1);
    }

    return opts;
}

VCSTagOptions get_vcs_tag_options(int argc, char * argv[]) {

    static const char * const short_opts = "h";
    static const option long_opts[] = {
        {"help", no_argument, nullptr, 'h'},
        {nullptr},
    };

    int c;
    while ((c = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (c) {
            case 'h':
            default:
                std::cout << usage << std::endl;
                exit(0);
        }
    }

    VCSTagOptions opts{};
    // ++ here to pass the verb
    int i = ++optind;
    if (argc - i < 6) {
        std::cerr << "meson++ vcs_tag command missing required command line options" << std::endl;
        std::cout << usage << std::endl;
        exit(1);
    }

    opts.infile = fs::path{argv[i++]};
    opts.outfile = fs::path{argv[i++]};
    opts.version = std::string{argv[i++]};
    opts.replacement = std::string{argv[i++]};
    opts.source_dir = fs::path{argv[i++]};
    opts.depfile = fs::path{argv[i++]};

    if (i < argc) {
        // TODO: better error message
        std::cerr << "Got extra arguments." << std::endl;
        std::cout << usage << std::endl;
        exit(1);
    }

    return opts;
}

} // namespace

OptionV parse_opts(int argc, char * argv[]) {
    if (argc < 1) {
        std::cerr << "Not enough arguments" << std::endl;
        std::cout << usage << std::endl;
    }
    // First we want to find which verb we're parsing
    auto && verb = get_verb(argc, argv);

    switch (verb) {
        case Verb::CONFIGURE:
            return get_config_options(argc, argv);
        case Verb::TEST:
            return get_test_options(argc, argv);
        case Verb::VCS_TAG:
            return get_vcs_tag_options(argc, argv);
        default:
            // TODO: should be unreachable
            throw std::runtime_error{"Unhandled verb"};
    }
}

} // namespace Options
