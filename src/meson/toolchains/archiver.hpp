// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interface for the static archiver, or static linker
 *
 * Meson++ uses "archiver" to distinguish this tool from the "linker", or
 * dynamic linker.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common.hpp"
#include "machines.hpp"

namespace Meson::Toolchain::Archiver {

/**
 * Abstract base for all Archivers.
 */
class Archiver {
  public:
    virtual ~Archiver(){};

    /**
     * What form (if any) or response file this archiver supports
     */
    virtual RSPFileSupport rsp_support() const = 0;
    virtual std::string id() const = 0;

  protected:
    Archiver(const std::vector<std::string> & c) : command{c} {};

    const std::vector<std::string> command;
};

/**
 * The GNU ar archiver.
 */
class Gnu : public Archiver {
  public:
    Gnu(const std::vector<std::string> & c) : Archiver{c} {};
    ~Gnu(){};

    RSPFileSupport rsp_support() const override;
    std::string id() const override { return "gnu"; }
};

/**
 * Find the static archiver to use
 */
std::unique_ptr<Archiver> detect_archiver(const Machines::Machine &,
                                          const std::vector<std::string> & bins = {});

} // namespace Meson::Toolchain::Archiver
