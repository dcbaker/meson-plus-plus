// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * All C++ Compilers, the interfaces
 */

#pragma once

#include "toolchains/compiler.hpp"

namespace HIR::Toolchain::Compiler::CPP {

class Gnu : public Compiler {
  public:
    Gnu(const std::vector<std::string> & c) : Compiler{c} {};
    ~Gnu(){};

    std::string id() const override { return "gcc"; };
    std::string language() const override { return "C++"; };

    RSPFileSupport rsp_support() const override;
};

class Clang : public Compiler {
  public:
    Clang(const std::vector<std::string> & c) : Compiler{c} {};
    ~Clang(){};

    std::string id() const override { return "clang"; };
    std::string language() const override { return "C++"; };

    RSPFileSupport rsp_support() const override;
};

} // namespace HIR::Toolchain::Compiler::CPP
