// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "toolchains/compilers/cpp/cpp.hpp"

namespace MIR::Toolchain::Compiler::CPP {

namespace fs = std::filesystem;

RSPFileSupport GnuLike::rsp_support() const { return RSPFileSupport::GCC; };
std::vector<std::string> GnuLike::output_command(const std::string & output) const {
    return {"-o", output};
}
std::vector<std::string> GnuLike::compile_only_command() const { return {"-c"}; }

Arguments::Argument GnuLike::generalize_argument(const std::string & arg) const {
    // XXX: this can't handle things like "-I foo"...
    const std::string start{arg.substr(0, 2)};

    if (start == "-L") {
        return {arg.substr(2, arg.size()), Arguments::Type::LINK_SEARCH};
    }
    if (start == "-D") {
        return {arg.substr(2, arg.size()), Arguments::Type::DEFINE};
    }
    if (start == "-l") {
        return {arg.substr(2, arg.size()), Arguments::Type::LINK};
    }
    if (start == "-I") {
        return {arg.substr(2, arg.size()), Arguments::Type::INCLUDE, Arguments::IncludeType::BASE};
    }
    if (start == "-isystem") {
        return {arg.substr(2, arg.size()), Arguments::Type::INCLUDE,
                Arguments::IncludeType::SYSTEM};
    }
    if (arg.substr(arg.length() - 2, arg.length()) == ".a") {
        return {arg, Arguments::Type::LINK};
    }
    if (arg.substr(arg.length() - 3, arg.length()) == ".so") {
        // TODO: or .so.X.Y.Z, .so.X.Y, .so.X
        return {arg, Arguments::Type::LINK};
    }
    return {arg, Arguments::Type::RAW};
}

std::vector<std::string> GnuLike::specialize_argument(const Arguments::Argument & arg,
                                                      const fs::path & src_dir,
                                                      const fs::path & build_dir) const {
    switch (arg.type) {
        case Arguments::Type::DEFINE:
            return {"-D", arg.value};
        case Arguments::Type::LINK:
            return {"-l", arg.value};
        case Arguments::Type::LINK_SEARCH:
            return {"-L", arg.value};
        case Arguments::Type::INCLUDE: {
            std::vector<std::string> args{};
            std::string inc_arg;
            switch (arg.inc_type) {
                case Arguments::IncludeType::BASE:
                    inc_arg = "-I";
                    break;
                case Arguments::IncludeType::SYSTEM:
                    inc_arg = "-system";
                    break;
                default:
                    throw std::exception{}; // Should be unreachable
            }
            std::string b_inc = "'" + std::string{fs::relative(arg.value, build_dir)} + "'";
            if (b_inc == "''") {
                b_inc = ".";
            }
            args.emplace_back(inc_arg);
            args.emplace_back(b_inc);
            args.emplace_back(inc_arg);
            // Needs to be relative to build dir
            args.emplace_back(std::string{fs::relative(src_dir / arg.value, build_dir)});
            return args;
        }
        case Arguments::Type::RAW:
            return {arg.value};
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

CanCompileType GnuLike::supports_file(const std::string & name) const {
    const auto ext = fs::path{name}.extension();
    if (ext == ".cpp" || ext == ".c++") {
        return CanCompileType::SOURCE;
    }
    if (ext == ".hpp" || ext == ".h++" || ext == ".h") {
        return CanCompileType::DEPENDS;
    }
    return CanCompileType::NONE;
}

std::vector<std::string> GnuLike::generate_depfile(const std::string & target_file,
                                                   const std::string & depfile) const {
    return {"-MD", "-MQ", target_file, "-MF", depfile};
}

} // namespace MIR::Toolchain::Compiler::CPP
