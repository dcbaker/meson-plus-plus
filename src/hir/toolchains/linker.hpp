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
    virtual std::string id() const = 0;

  protected:
    Linker(const std::vector<std::string> & c) : command{c} {};
    const std::vector<std::string> command;
};

class GnuBFD : public Linker {
  public:
    GnuBFD(const std::vector<std::string> & c) : Linker{c} {};
    virtual ~GnuBFD(){};

    std::string id() const override { return "ld.bfd"; }
    RSPFileSupport rsp_support() const override final;
};

namespace Drivers {

// TODO: this might have to be a templateized class
class Gnu : Linker {
  public:
    Gnu(const std::vector<std::string> & s, const GnuBFD & l) : Linker{s}, linker{l} {};
    virtual ~Gnu(){};

    std::string id() const override { return linker.id(); }
    RSPFileSupport rsp_support() const override final;

  private:
    const GnuBFD linker;
};

} // namespace Drivers

} // namespace HIR::Toolchain::Linker
