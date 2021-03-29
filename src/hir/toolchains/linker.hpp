// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interfacce for linkers
 *
 * Meson++ uses the term "linker" for dynamic linkers, those that create
 * exectuables and loadable libraries (.dll, .so, .dylib, etc)
 */

#pragma once

#include <string>
#include <vector>

#include "common.hpp"

namespace HIR::Toolchain::Linker {

/**
 * Abstract base for all Linkers.
 */
class Linker {
  public:
    virtual ~Linker(){};
    virtual RSPFileSupport rsp_support() const = 0;

  protected:
    Linker(const std::vector<std::string> & c) : command{c} {};
    const std::vector<std::string> command;
};

} // namespace HIR::Toolchain::Linker
