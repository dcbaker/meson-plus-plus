// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include "arguments.hpp"
#include "machines.hpp"
#include "toolchains/toolchain.hpp"

namespace fs = std::filesystem;

namespace MIR::State {

/**
 * Serializable representation of a Dependency
 */
class Dependency {
  public:
    Dependency();
    Dependency(std::string ver, std::vector<Arguments::Argument> compile_args,
               std::vector<Arguments::Argument> link_args);

    /// The version of the dependency
    std::string version;

    /// compilation arguments for this dependency
    std::vector<Arguments::Argument> compile{};

    /// link arguments for this dependency
    std::vector<Arguments::Argument> link{};

    /// whether the dependency is found or not
    bool found;
};

/**
 * Persistant state
 *
 * This state is meant to persist across reconfigurations
 */
class Persistant {
  public:
    Persistant(std::filesystem::path, std::filesystem::path);

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
    Machines::PerMachine<std::unordered_map<std::string, fs::path>> programs;

    /**
     * Programs found by the `dependency` function, as well as `compiler.find_program`.
     *
     * These are cached across re-runs.
     * These are stored int [name: Dependency] format, an actual representation has to
     * be built when getting a value from the cache.
     *
     * TODO: what do we need to do about multiple versions of a dependency?
     */
    Machines::PerMachine<std::unordered_map<std::string, std::shared_ptr<Dependency>>> dependencies;
};

} // namespace MIR::State
