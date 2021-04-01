// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#pragma once

#include <string>
#include <vector>

/**
 * Interface for all targets
 */

namespace HIR::Target {

/**
 * A Compiled target
 *
 */
class Compiled {
  public:
    Compiled(const std::string & n, const std::string & s, const std::vector<std::string> & srcs)
        : name{n}, subdir{s}, sources{srcs} {};
    ~Compiled(){};

  private:
    const std::string name;
    const std::string subdir;
    const std::vector<std::string> sources;
};

} // namespace HIR::Target
