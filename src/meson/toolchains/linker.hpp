// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interfacce for linkers
 *
 * Meson++ uses the term "linker" for dynamic linkers, those that create
 * exectuables and loadable libraries (.dll, .so, .dylib, etc)
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common.hpp"
#include "compiler.hpp"

namespace Meson::Toolchain::Linker {

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
    ~GnuBFD(){};

    std::string id() const override { return "ld.bfd"; }
    RSPFileSupport rsp_support() const override final;
};

namespace Drivers {

// TODO: this might have to be a templateized class
class Gnu : public Linker {
  public:
    Gnu(const GnuBFD & l) : Linker{{}}, linker{l} {};
    ~Gnu(){};

    std::string id() const override { return linker.id(); }
    RSPFileSupport rsp_support() const override final;

  private:
    const GnuBFD linker;
};

} // namespace Drivers

std::unique_ptr<Linker> detect_linker(const std::unique_ptr<Compiler::Compiler> & comp,
                                      const Machines::Machine & machine);

} // namespace Meson::Toolchain::Linker
