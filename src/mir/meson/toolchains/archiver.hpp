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

namespace MIR::Toolchain::Archiver {

/**
 * Abstract base for all Archivers.
 */
class Archiver {
  public:
    virtual ~Archiver() = default;

    /**
     * What form (if any) or response file this archiver supports
     */
    virtual RSPFileSupport rsp_support() const = 0;
    virtual std::string id() const = 0;
    virtual std::vector<std::string> command() const = 0;

    /// Arguments that should always be used by this langauge/compiler
    virtual std::vector<std::string> always_args() const = 0;

    Archiver(std::vector<std::string> c) : _command{std::move(c)} {};

    const std::vector<std::string> _command;
};

/**
 * The GNU ar archiver.
 */
class Gnu : public Archiver {
    using Archiver::Archiver;

  public:
    RSPFileSupport rsp_support() const override;
    std::string id() const override { return "gnu"; }
    std::vector<std::string> command() const final;
    std::vector<std::string> always_args() const final;
};

/**
 * Find the static archiver to use
 */
std::unique_ptr<Archiver> detect_archiver(const Machines::Machine &,
                                          const std::vector<std::string> & bins = {});

} // namespace MIR::Toolchain::Archiver
