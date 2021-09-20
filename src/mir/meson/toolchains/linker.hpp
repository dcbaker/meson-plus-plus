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

namespace MIR::Toolchain::Linker {

/**
 * Abstract base for all Linkers.
 */
class Linker {
  public:
    virtual ~Linker(){};
    virtual RSPFileSupport rsp_support() const = 0;
    virtual std::string id() const = 0;

    /// Get the pretty language output
    virtual std::string language() const = 0;

    /**
     * Get the command line arguments to set the output of the linker
     *
     * @param outfile The name of the file to output.
     */
    virtual std::vector<std::string> output_command(const std::string & outfile) const = 0;

    /// Get the command for this linker
    virtual const std::vector<std::string> command() const = 0;

    /// Get arguments that should always be used for this linker
    virtual std::vector<std::string> always_args() const = 0;

  protected:
    Linker(const std::vector<std::string> & c) : _command{c} {};
    const std::vector<std::string> _command;
};

class GnuBFD : public Linker {
  public:
    GnuBFD(const std::vector<std::string> & c) : Linker{c} {};
    ~GnuBFD(){};

    std::string id() const override { return "ld.bfd"; }
    RSPFileSupport rsp_support() const override final;
    std::string language() const final {
        throw std::exception{}; // "Should be unused"
    }
    std::vector<std::string> output_command(const std::string & outfile) const final {
        throw std::exception{}; // "Should be unused"
    }
    const std::vector<std::string> command() const final { return _command; }
    std::vector<std::string> always_args() const final { return {}; }
};

namespace Drivers {

// TODO: this might have to be a templateized class
class Gnu : public Linker {
  public:
    Gnu(const GnuBFD & l, const Compiler::Compiler * const c)
        : Linker{{}}, linker{l}, compiler{c} {};
    ~Gnu(){};

    std::string id() const override { return linker.id(); }
    RSPFileSupport rsp_support() const override final;
    std::string language() const override;
    std::vector<std::string> output_command(const std::string & outfile) const override;
    const std::vector<std::string> command() const final { return compiler->command; }
    std::vector<std::string> always_args() const final { return {}; }

  private:
    const GnuBFD linker;
    const Compiler::Compiler * const compiler;
};

} // namespace Drivers

std::unique_ptr<Linker> detect_linker(const std::unique_ptr<Compiler::Compiler> & comp,
                                      const Machines::Machine & machine);

} // namespace MIR::Toolchain::Linker
