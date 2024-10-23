// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "state.hpp"
#include "exceptions.hpp"
#include "utils.hpp"

#include <array>
#include <cassert>
#include <string>
#include <vector>

namespace MIR::State {

Persistant::Persistant() : machines{Machines::detect_build()} {};

Persistant::Persistant(std::filesystem::path sr_, std::filesystem::path br_)
    : machines{Machines::detect_build()}, source_root{std::move(sr_)},
      build_root{std::move(br_)} {};

void Persistant::serialize(std::ostream & out) const {
    out << "name:" << name << '\n'
        << "source root:" << std::string{source_root} << '\n'
        << "build root:" << std::string{build_root} << '\n'
        << "project_version:" << std::string{project_version} << '\n';
    // TODO: toolchains
    // TODO: machines
    // TODO: programs
}

Persistant load(std::istream & in) {
    Persistant pstate{};
    fs::path src, bld;
    // This size is arbitrary
    for (std::array<char, 1024> chars; in.getline(&chars[0], 1024, '\n');) {
        std::vector<std::string> split = Util::split(chars.data(), ":");
        if (split.size() != 2) {
            throw Util::Exceptions::MesonException(
                "Malformed line in Persistant state serialization: " + std::string{chars.data()});
        }
        const std::string & k = split[0];
        const std::string & v = split[1];

        if (k == "source root") {
            pstate.source_root = v;
        } else if (k == "build root") {
            pstate.build_root = v;
        } else if (k == "name") {
            pstate.name = v;
        } else if (k == "project_version") {
            pstate.project_version = v;
        }
    }

    return pstate;
}

} // namespace MIR::State
