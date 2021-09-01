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
    const bool is_built() const;
    const std::string get_name() const;
    const std::filesystem::path relative_to_source_dir() const;
    const std::filesystem::path relative_to_build_dir() const;

  private:
    const std::string name;
    const bool built;
    const std::filesystem::path source_root;
    const std::filesystem::path build_root;
};

} // namespace MIR::Objects
