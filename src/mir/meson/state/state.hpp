// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#pragma once

#include <filesystem>
#include <unordered_map>

#include "machines.hpp"
#include "toolchains/toolchain.hpp"

namespace fs = std::filesystem;

namespace MIR::State {

/**
 * Persistant state
 *
 * This state is meant to persist across reconfigurations
 */
class Persistant {
  public:
    Persistant(const std::filesystem::path &, const std::filesystem::path &);

    // This must be mutable because of `add_language`
    /// A mapping of language : machine : toolchain
    std::unordered_map<Toolchain::Language,
                       Machines::PerMachine<std::shared_ptr<Toolchain::Toolchain>>>
        toolchains;

    /// The information on each machine
    /// XXX: currently only handle host == build configurations, as we don't have
    /// a machine file
    Machines::PerMachine<Machines::Info> machines;

    /// absolute path to the source tree
    const std::filesystem::path source_root;

    /// absolute path to the build tree
    const std::filesystem::path build_root;

    /// The name of the project
    std::string name;

    /**
     * Programs found by the `find_program` function. These are cached across re-runs
     *
     * These are stored int [str: path] format, an actual representation has to
     * be built when getting a value from the cache.
     */
    Machines::PerMachine<
        std::unordered_map<std::string, std::tuple<fs::path, std::vector<std::string>>>>
        programs;
};

} // namespace MIR::State
