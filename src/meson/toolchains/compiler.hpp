// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interface for the Compiler class.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common.hpp"
#include "machines.hpp"

namespace Meson::Toolchain::Compiler {

/**
 * Abstract base for all Compilers.
 */
class Compiler {
  public:
    virtual ~Compiler(){};
    virtual RSPFileSupport rsp_support() const = 0;
    virtual std::string id() const = 0;
    virtual std::string language() const = 0;

    const std::vector<std::string> command;

  protected:
    Compiler(const std::vector<std::string> & c) : command{c} {};
};

std::unique_ptr<Compiler> detect_compiler(const Language &, const Machines::Machine &,
                                          const std::vector<std::string> & bins = {});

} // namespace HIR::Toolchain::Compiler
