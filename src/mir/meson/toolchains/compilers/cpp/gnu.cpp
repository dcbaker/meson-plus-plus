// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/compilers/cpp/cpp.hpp"

namespace MIR::Toolchain::Compiler::CPP {

RSPFileSupport Gnu::rsp_support() const { return RSPFileSupport::GCC; };
std::vector<std::string> Gnu::output_command(const std::string & output) const {
    return {"-o", output};
}
std::vector<std::string> Gnu::compile_only_command() const { return {"-c"}; }

} // namespace MIR::Toolchain::Compiler::CPP
