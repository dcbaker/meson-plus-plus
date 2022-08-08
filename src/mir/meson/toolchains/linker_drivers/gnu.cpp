// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/linker.hpp"

namespace MIR::Toolchain::Linker::Drivers {

RSPFileSupport Gnu::rsp_support() const { return linker.rsp_support(); }

std::string Gnu::language() const { return compiler->language(); }

std::vector<std::string> Gnu::output_command(const std::string & outfile) const {
    return compiler->output_command(outfile);
}

std::vector<std::string> Gnu::specialize_argument(const Arguments::Argument & arg,
                                                  const fs::path & src_dir,
                                                  const fs::path & build_dir) const {
    return linker.specialize_argument(arg, src_dir, build_dir);
}

} // namespace MIR::Toolchain::Linker::Drivers
