// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interfacce for linkers
 *
 * Meson++ uses the term "linker" for dynamic linkers, those that create
 * exectuables and loadable libraries (.dll, .so, .dylib, etc)
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "common.hpp"
#include "compiler.hpp"

namespace MIR::Toolchain::Linker {

namespace fs = std::filesystem;

/**
 * Abstract base for all Linkers.
 */
class Linker {
  public:
    Linker(const Linker &) = default;
    virtual ~Linker() = default;
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
    virtual const std::vector<std::string> & command() const = 0;

    /// Get arguments that should always be used for this linker
    virtual std::vector<std::string> always_args() const = 0;
    /**
     * Convert a generic argument into a compiler specific one one
     *
     * @param arg The Argument to be converted
     * @param src_dir the path of the source directory
     * @param build_dir the path of the source directory
     */
    virtual std::vector<std::string> specialize_argument(const Arguments::Argument & arg,
                                                         const fs::path & src_dir,
                                                         const fs::path & build_dir) const = 0;

    Linker(std::vector<std::string> c) : _command{std::move(c)} {};
    const std::vector<std::string> _command;
};

class GnuBFD : public Linker {
    using Linker::Linker;

  public:
    std::string id() const override { return "ld.bfd"; }
    RSPFileSupport rsp_support() const final;
    std::string language() const final {
        throw std::exception{}; // "Should be unused"
    }
    std::vector<std::string> output_command(const std::string & outfile) const final {
        throw std::exception{}; // "Should be unused"
    }
    const std::vector<std::string> & command() const final { return _command; }
    std::vector<std::string> always_args() const final { return {}; }
    std::vector<std::string> specialize_argument(const Arguments::Argument & arg,
                                                 const fs::path & src_dir,
                                                 const fs::path & build_dir) const final;
};

namespace Drivers {

// TODO: this might have to be a templateized class
class Gnu : public Linker {
  public:
    Gnu(GnuBFD l, const Compiler::Compiler * const c)
        : Linker{{}}, linker{std::move(l)}, compiler{c} {};

    std::string id() const override { return linker.id(); }
    RSPFileSupport rsp_support() const final;
    std::string language() const override;
    std::vector<std::string> output_command(const std::string & outfile) const override;
    const std::vector<std::string> & command() const final { return compiler->command; }
    std::vector<std::string> always_args() const final { return {}; }
    std::vector<std::string> specialize_argument(const Arguments::Argument & arg,
                                                 const fs::path & src_dir,
                                                 const fs::path & build_dir) const final;

  private:
    const GnuBFD linker;
    const Compiler::Compiler * const compiler;
};

} // namespace Drivers

std::unique_ptr<Linker> detect_linker(const std::unique_ptr<Compiler::Compiler> & comp,
                                      const Machines::Machine & machine);

} // namespace MIR::Toolchain::Linker
