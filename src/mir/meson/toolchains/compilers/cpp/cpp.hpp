// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * All C++ Compilers, the interfaces
 */

#pragma once

#include <filesystem>

#include "toolchains/compiler.hpp"

namespace MIR::Toolchain::Compiler::CPP {

namespace fs = std::filesystem;

class GnuLike : public Compiler {
  public:
    RSPFileSupport rsp_support() const final;
    std::vector<std::string> compile_only_command() const final;
    std::vector<std::string> output_command(const std::string &) const final;
    Arguments::Argument generalize_argument(const std::string &) const final;
    std::vector<std::string> specialize_argument(const Arguments::Argument & arg,
                                                 const fs::path & src_dir,
                                                 const fs::path & build_dir) const final;
    std::vector<std::string> always_args() const final;

  protected:
    GnuLike(const std::vector<std::string> & c) : Compiler{c} {};
};

class Gnu : public GnuLike {
  public:
    Gnu(const std::vector<std::string> & c) : GnuLike{c} {};
    ~Gnu(){};

    std::string id() const override { return "gcc"; };
    std::string language() const override { return "C++"; };
};

class Clang : public GnuLike {
  public:
    Clang(const std::vector<std::string> & c) : GnuLike{c} {};
    ~Clang(){};

    std::string id() const override { return "clang"; };
    std::string language() const override { return "C++"; };
};

} // namespace MIR::Toolchain::Compiler::CPP
