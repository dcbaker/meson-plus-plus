// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* GCC/G++ driving a linker
 */

#pragma once

#include "toolchains/linker.hpp"
#include "toolchains/linkers/gnu.hpp"

namespace HIR::Toolchain::Linker::Drivers {

// TODO: this might have to be a templateized class
class Gnu : Linker {
  public:
    Gnu(const std::vector<std::string> & s, const GnuBFD & l) : Linker{s}, linker{l} {};
    virtual ~Gnu(){};

    RSPFileSupport rsp_support() const override final;

  private:
    const GnuBFD linker;
};

} // namespace HIR::Toolchain::Linker::Drivers
