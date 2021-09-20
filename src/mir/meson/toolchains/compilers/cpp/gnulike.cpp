// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/compilers/cpp/cpp.hpp"

namespace MIR::Toolchain::Compiler::CPP {

RSPFileSupport GnuLike::rsp_support() const { return RSPFileSupport::GCC; };
std::vector<std::string> GnuLike::output_command(const std::string & output) const {
    return {"-o", output};
}
std::vector<std::string> GnuLike::compile_only_command() const { return {"-c"}; }

Arguments::Argument GnuLike::generalize_argument(const std::string & arg) const {
    if (arg.substr(0, 2) == "-L") {
        return Arguments::Argument(arg.substr(2, arg.size()), Arguments::Type::LINK_SEARCH);
    } else if (arg.substr(0, 2) == "-D") {
        return Arguments::Argument(arg.substr(2, arg.size()), Arguments::Type::DEFINE);
    } else if (arg.substr(0, 2) == "-l") {
        return Arguments::Argument(arg.substr(2, arg.size()), Arguments::Type::LINK);
    } else if (arg.substr(arg.length() - 2, arg.length()) == ".a") {
        return Arguments::Argument(arg, Arguments::Type::LINK);
    } else if (arg.substr(arg.length() - 2, arg.length()) == ".so") {
        // TODO: or .so.X.Y.Z, .so.X.Y, .so.X
        return Arguments::Argument(arg, Arguments::Type::LINK);
    } else {
        return Arguments::Argument(arg, Arguments::Type::RAW);
    }
}

std::string GnuLike::specialize_argument(const Arguments::Argument & arg) const {
    switch (arg.type) {
        case Arguments::Type::DEFINE:
            return "-D" + arg.value;
        case Arguments::Type::LINK:
            return "-l" + arg.value;
        case Arguments::Type::LINK_SEARCH:
            return "-L" + arg.value;
        case Arguments::Type::RAW:
            return arg.value;
        default:
            throw std::exception{}; // Should be unreachable
    }
}

std::vector<std::string> GnuLike::always_args() const {
    std::vector<std::string> args{};

    // TODO: if not darwin
    args.emplace_back("-D_FILE_OFFSET_BITS=64");

    return args;
}

} // namespace MIR::Toolchain::Compiler::CPP
