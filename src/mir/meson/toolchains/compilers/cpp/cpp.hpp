// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * All C++ Compilers, the interfaces
 */

#pragma once

#include <filesystem>

#include "toolchains/compiler.hpp"

namespace MIR::Toolchain::Compiler::CPP {

namespace fs = std::filesystem;

class GnuLike : public Compiler {
    using Compiler::Compiler;

  public:
    RSPFileSupport rsp_support() const final;
    std::vector<std::string> compile_only_command() const final;
    std::vector<std::string> output_command(const std::string &) const final;
    Arguments::Argument generalize_argument(const std::string &) const final;
    std::vector<std::string> specialize_argument(const Arguments::Argument & arg,
                                                 const fs::path & src_dir,
                                                 const fs::path & build_dir) const final;
    std::vector<std::string> always_args() const final;
    CanCompileType supports_file(const std::string &) const final;
    std::vector<std::string> generate_depfile(const std::string &, const std::string &) const final;
};

class Gnu : public GnuLike {
    using GnuLike::GnuLike;

  public:
    std::string id() const override { return "gcc"; };
    std::string language() const override { return "C++"; };
};

class Clang : public GnuLike {
    using GnuLike::GnuLike;

  public:
    std::string id() const override { return "clang"; };
    std::string language() const override { return "C++"; };
};

} // namespace MIR::Toolchain::Compiler::CPP
