// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Compiler detection functions
 */

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "archiver.hpp"
#include "process.hpp"

namespace Meson::Toolchain::Archiver {

namespace {

const std::vector<std::string> DEFAULT{"ar"};

}

std::unique_ptr<Archiver> detect_archiver(const Machines::Machine & machine, const std::vector<std::string> & bins) {
    // TODO: handle the machine switch, and the cross/native file
    for (const auto & c : bins.empty() ? DEFAULT : bins) {
        auto const & [ret, out, err] = Util::process(std::vector<std::string>{c, "--version"});
        if (ret != 0) { continue; }

        if (out.find("Free Software Foundation") != std::string::npos) {
            return std::make_unique<Gnu>(std::vector<std::string>{c});
        }
    }
    return nullptr;
};

} // namespace HIR::Toolchain::Compiler
