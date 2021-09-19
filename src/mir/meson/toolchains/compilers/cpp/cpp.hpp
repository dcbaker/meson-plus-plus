// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * All C++ Compilers, the interfaces
 */

#pragma once

#include "toolchains/compiler.hpp"

namespace MIR::Toolchain::Compiler::CPP {

class GnuLike : public Compiler {
  protected:
    GnuLike(const std::vector<std::string> & c) : Compiler{c} {};
};

class Gnu : public GnuLike {
  public:
    Gnu(const std::vector<std::string> & c) : GnuLike{c} {};
    ~Gnu(){};

    std::string id() const override { return "gcc"; };
    std::string language() const override { return "C++"; };

    RSPFileSupport rsp_support() const override;
    std::vector<std::string> compile_only_command() const override;
    std::vector<std::string> output_command(const std::string &) const override;
};

class Clang : public GnuLike {
  public:
    Clang(const std::vector<std::string> & c) : GnuLike{c} {};
    ~Clang(){};

    std::string id() const override { return "clang"; };
    std::string language() const override { return "C++"; };

    RSPFileSupport rsp_support() const override;
    std::vector<std::string> compile_only_command() const override;
    std::vector<std::string> output_command(const std::string &) const override;
};

} // namespace MIR::Toolchain::Compiler::CPP
