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

namespace HIR::Toolchain::Linker {

/**
 * Abstract base for all Linkers.
 */
class Linker {
  public:
    virtual ~Linker(){};

  private:
    const std::vector<std::string> command;
};

} // namespace HIR::Toolchain::Linker
