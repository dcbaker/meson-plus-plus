// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Interface for the Compiler class.
 */

#include <optional>
#include <regex>
#include <string>

#include "compiler.hpp"

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

} // namespace

Compiler & detect_compiler(const Language &, const Machines::Machine &){};

} // namespace HIR::Toolchain::Compiler
