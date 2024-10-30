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
    VCS_TAG,
};

/**
 * Options for the configure command
 */
struct ConfigureOptions {
    std::string program;
    fs::path sourcedir;
    fs::path builddir;
    std::unordered_map<std::string, std::string> options;
};

/**
 * Options for the test subcommand
 */
struct TestOptions {
    fs::path builddir;
};

/**
 * @brief Options for the vcs_tag command
 */
struct VCSTagOptions {
    /// @brief the input template file
    fs::path infile;
    /// @brief the output file name
    fs::path outfile;
    /// @brief the version string to use
    std::string version;
    /// @brief the string to be replaced
    std::string replacement;
    /// @brief  The absolute path to the source dir
    fs::path source_dir;
};

using OptionV = std::variant<ConfigureOptions, TestOptions, VCSTagOptions>;

/// Parse options and return an Options object
OptionV parse_opts(int argc, char * argv[]);

} // namespace Options
