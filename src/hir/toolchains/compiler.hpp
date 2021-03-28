// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interface for the Compiler class.
 */

#pragma once

#include <string>
#include <vector>

#include "common.hpp"

namespace HIR::Toolchain::Compiler {

/**
 * Abstract base for all Compilers.
 */
class Compiler {
  public:
    virtual ~Compiler(){};
    virtual RSPFileSupport rsp_support() const;

  protected:
    Compiler(const std::vector<std::string> & c) : command{c} {};

    const std::vector<std::string> command;
};

} // namespace HIR::Toolchain::Compiler
