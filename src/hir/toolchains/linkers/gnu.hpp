// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Gnu ld.bfd linker
 */

#pragma once

#include "toolchains/linker.hpp"

namespace HIR::Toolchain::Linker {

class GnuBFD : public Linker {
  public:
    GnuBFD(const std::vector<std::string> & c) : Linker{c} {};
    virtual ~GnuBFD(){};

    RSPFileSupport rsp_support() const override final;
};

} // namespace HIR::Toolchain::Linker
