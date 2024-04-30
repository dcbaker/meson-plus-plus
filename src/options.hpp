// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Meson++ argument parsing
 */

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>

namespace {

namespace fs = std::filesystem;

}

namespace Options {

/// Which action we're taking
enum class Verb {
    CONFIGURE,
    TEST,
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
 * Options for the test subcommand
 */
struct TestOptions {
    fs::path builddir;
};

using OptionV = std::variant<ConfigureOptions, TestOptions>;

/// Parse options and return an Options object
OptionV parse_opts(int argc, char * argv[]);

} // namespace Options
