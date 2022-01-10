// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interface for the Compiler class.
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "arguments.hpp"
#include "common.hpp"
#include "machines.hpp"

namespace MIR::Toolchain::Compiler {

namespace fs = std::filesystem;

/**
 * Abstract base for all Compilers.
 */
class Compiler {
  public:
    virtual ~Compiler(){};
    virtual RSPFileSupport rsp_support() const = 0;
    virtual std::string id() const = 0;

    /// Get the pretty language output
    virtual std::string language() const = 0;

    /// Get the command line arguments to compile only, without linking
    virtual std::vector<std::string> compile_only_command() const = 0;

    /// Arguments that should always be used by this langauge/compiler
    virtual std::vector<std::string> always_args() const = 0;

    /**
     * Get the command line arguments to set the output of the compiler
     *
     * @param outfile The name of the file to output.
     */
    virtual std::vector<std::string> output_command(const std::string & outfile) const = 0;

    /**
     * Convert a compiler specific argument into a generic one
     *
     * @param arg The Argument to be converted
     */
    virtual Arguments::Argument generalize_argument(const std::string &) const = 0;

    /**
     * Convert a generic argument into a compiler specific one one
     *
     * @param arg The Argument to be converted
     */
    virtual std::string specialize_argument(const Arguments::Argument & arg) const = 0;

    /**
     * Create include arguments for a build and source directory
     *
     * @param dir The directory to be included
     * @param sdir the source directory root
     * @param bdir the build directory root
     */
    virtual std::vector<std::string> include_directories(const std::string & dir,
                                                         const fs::path & sdir,
                                                         const fs::path & bdir) const = 0;

    /// Command to invoke this compiler, as a vector
    const std::vector<std::string> command;

  protected:
    Compiler(const std::vector<std::string> & c) : command{c} {};
}; // namespace std::filesystemclassCompiler

std::unique_ptr<Compiler> detect_compiler(const Language &, const Machines::Machine &,
                                          const std::vector<std::string> & bins = {});

} // namespace MIR::Toolchain::Compiler
