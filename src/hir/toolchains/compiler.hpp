// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interface for the Compiler class.
 */

#pragma once

#include <string>
#include <vector>

#include "common.hpp"
#include "machines.hpp"

namespace HIR::Toolchain::Compiler {

/**
 * Abstract base for all Compilers.
 */
class Compiler {
  public:
    virtual ~Compiler(){};
    virtual RSPFileSupport rsp_support() const = 0;

  protected:
    Compiler(const std::vector<std::string> & c) : command{c} {};

    const std::vector<std::string> command;
};

Compiler & detect_compiler(const Language &, const Machines::Machine &);

} // namespace HIR::Toolchain::Compiler
