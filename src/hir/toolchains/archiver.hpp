// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interface for the static archiver, or static linker
 *
 * Meson++ uses "archiver" to distinguish this tool from the "linker", or
 * dynamic linker.
 */

#pragma once

#include <string>
#include <vector>

namespace HIR::Toolchain::Archiver {

/**
 * Abstract base for all Archivers.
 */
class Archiver {
  public:
    virtual ~Archiver(){};
    virtual bool accepts_rsp_file() const;

  protected:
    Archiver(const std::vector<std::string> & c) : command{c} {};

    const std::vector<std::string> command;
};

} // namespace HIR::Toolchain::Archiver
