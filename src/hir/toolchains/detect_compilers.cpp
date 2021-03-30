// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Compiler detection functions
 */

#include <cassert>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "compiler.hpp"
#include "compilers/cpp/cpp.hpp"
#include "process.hpp"

namespace HIR::Toolchain::Compiler {

namespace {

std::optional<std::string> detect_version(const std::string & raw) {
    std::regex re{"\\d\\.\\d\\.\\d"};
    std::smatch match{};
    if (std::regex_search(raw, match, re)) {
        return match.str(1);
    } else {
        return std::nullopt;
    }
};

const std::vector<std::string> DEFAULT_CPP{"c++", "g++"};

std::unique_ptr<Compiler> detect_cpp_compiler(const Machines::Machine &) {
    // TODO: handle the machine switch, and the cross/native file
    for (const auto & c : DEFAULT_CPP) {
        auto const & [ret, out, err] = Util::process(std::vector<std::string>{c, "--version"});
        if (ret != 0) { continue; }

        if (out.find("Free Software Foundation") != std::string::npos) {
            return std::make_unique<CPP::Gnu>(std::vector<std::string>{c});
        }
    }
    return nullptr;
}

} // namespace

std::unique_ptr<Compiler> detect_compiler(const Language & lang, const Machines::Machine & machine) {
    switch (lang) {
        case Language::CPP:
            return detect_cpp_compiler(machine);
    }
    assert(false);
};

} // namespace HIR::Toolchain::Compiler
