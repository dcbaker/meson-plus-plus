// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Implementation of g++, the Gnu C++ Compiler
 */

#pragma once

#include "toolchains/compiler.hpp"

namespace HIR::Toolchain::Compiler::CPP {

class Gnu : public Compiler {
  public:
    Gnu(const std::vector<std::string> & c) : Compiler{c} {};
    ~Gnu(){};

    std::string id() const override { return "gcc"; };

    RSPFileSupport rsp_support() const override;
};

} // namespace HIR::Toolchain::Compiler::CPP
