// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/**
 * Meson++ argument parsing
 */

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

namespace {

namespace fs = std::filesystem;

}

namespace Options {

/// Which action we're taking
enum class Verb {
    CONFIGURE,
};

/**
 * Options for the configure command
 */
struct ConfigureOptions {
    fs::path builddir;
    fs::path sourcedir;
    std::unordered_map<std::string, std::string> options;
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
