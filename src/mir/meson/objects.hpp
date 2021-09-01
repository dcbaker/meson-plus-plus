// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Generic objects
 *
 * Things that are not complex or big enough to deserve their own files
 */

#pragma once

#include <string>

namespace MIR::Objects {

class File {
  public:
    File(const std::string & name_, const bool & built_) : name{name_}, built{built_} {};
    const bool is_built() const;

  private:
    const std::string name;
    const bool built;
};

} // namespace MIR::Objects
