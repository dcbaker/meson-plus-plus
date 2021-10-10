// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Generic objects
 *
 * Things that are not complex or big enough to deserve their own files
 */

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "arguments.hpp"
#include "machines.hpp"
#include "toolchains/toolchain.hpp"

namespace fs = std::filesystem;

namespace MIR::Objects {

using ArgMap = std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>>;

/**
 * A Meson File, which is a smart object that knows its location relative to the
 * source and build directories
 */
class File {
  public:
    File(const std::string & name_, const fs::path & sdir, const bool & built_,
         const fs::path & sr_, const fs::path & br_)
        : name{name_}, subdir{sdir}, built{built_}, source_root{sr_}, build_root{br_} {};

    /// Whether this is a built object, or a static one
    const bool is_built() const;

    /// Get the name of the of the file, relative to the src dir if it's static,
    /// or the build dir if it's built
    const std::string get_name() const;

    /// Get a path for this file relative to the source tree
    const fs::path relative_to_source_dir() const;

    /// Get a path for this file relative to the build treeZ
    const fs::path relative_to_build_dir() const;

    bool operator==(const File &) const;
    bool operator!=(const File &) const;

    // For gtest
    friend std::ostream & operator<<(std::ostream & os, const File & f);

  private:
    const std::string name;
    const fs::path subdir;
    const bool built;
    const fs::path source_root;
    const fs::path build_root;
};

/**
 * A Base build Target
 *
 * Meant to be shared by other build target classes
 */
class BuildTarget {
  public:
    /// The name of the target
    const std::string name;

    /// The sources (as files)
    const std::vector<File> sources;

    /// Which machine is this executable to be built for?
    const Machines::Machine machine;

    /**
     * Arguments for the target, sorted by langauge
     *
     * We sort these by language, as each compiled source will only recieve it's
     * per-language arguments
     */
    const ArgMap arguments;

  protected:
    BuildTarget(const std::string & name_, const std::vector<File> & srcs,
                const Machines::Machine & m, const ArgMap & args)
        : name{name_}, sources{srcs}, machine{m}, arguments{args} {};
};

/**
 * An Executable Target
 */
class Executable : public BuildTarget {
  public:
    Executable(const std::string & name_, const std::vector<File> & srcs,
               const Machines::Machine & m, const ArgMap & args)
        : BuildTarget{name_, srcs, m, args} {};
};

/**
 * A Static Library
 */
class StaticLibrary : public BuildTarget {
  public:
    StaticLibrary(const std::string & name_, const std::vector<File> & srcs,
                  const Machines::Machine & m, const ArgMap & args)
        : BuildTarget{name_, srcs, m, args} {};
};

} // namespace MIR::Objects
