// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/**
 * Meson++ argument parsing
 */

#pragma once

#include <string>

namespace Options {

/// Which action we're taking
enum class Verb {
    CONFIGURE,
};

/**
 * Options for the configure command
 */
struct ConfigureOptions {
    std::string builddir;
};

/**
 * Commandline options to execute
 */
struct Options {
    Verb verb;
    // TODO: probably this should be stored in a union of some kind?
    ConfigureOptions config;
};

/// Parse options and return an Options object
Options parse_opts(int argc, char * argv[]);

} // namespace Options
