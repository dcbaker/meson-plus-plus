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

namespace MIR::Objects {

class File {
  public:
    File(const std::string & name_, const bool & built_, const std::filesystem::path & sr_,
         const std::filesystem::path & br_)
        : name{name_}, built{built_}, source_root{sr_}, build_root{br_} {};

    /// Whether this is a built object, or a static one
    const bool is_built() const;

    /// Get the name of the of the file, relative to the src dir if it's static,
    /// or the build dir if it's built
    const std::string get_name() const;

    /// Get a path for this file relative to the source tree
    const std::filesystem::path relative_to_source_dir() const;

    /// Get a path for this file relative to the build treeZ
    const std::filesystem::path relative_to_build_dir() const;

  private:
    const std::string name;
    const bool built;
    const std::filesystem::path source_root;
    const std::filesystem::path build_root;
};

} // namespace MIR::Objects
