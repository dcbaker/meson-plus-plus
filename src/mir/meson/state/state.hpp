// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#pragma once

#include <unordered_map>

#include "machines.hpp"
#include "toolchains/toolchain.hpp"

namespace MIR::State {

/**
 * Persistant state
 *
 * This state is meant to persist across reconfigurations
 */
class Persistant {
  public:
    Persistant() : toolchains{}, machines{Machines::detect_build()} {};
    ~Persistant(){};

    // This must be mutable because of `add_language`
    /// A mapping of language : machine : toolchain
    std::unordered_map<Toolchain::Language, Machines::PerMachine<Toolchain::Toolchain>> toolchains;

    /// The information on each machine
    /// XXX: currently only handle host == build configurations, as we don't have
    /// a machine file
    Machines::PerMachine<Machines::Info> machines;
};

} // namespace MIR::State
