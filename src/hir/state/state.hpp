// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#pragma once

#include <unordered_map>

#include "machines.hpp"
#include "toolchains/toolchain.hpp"

namespace HIR::State {

/**
 * Persistant state
 *
 * This state is meant to persist across reconfigurations
 */
class Persistant {
  public:
    Persistant() : toolchains{}, machines{Machines::detect_build()} {};
    ~Persistant() {};

    /// A mapping of language : machine : toolchain
    /// XXX: this might need a way to make each machine optional
    std::unordered_map<Toolchain::Language, Machines::PerMachine<Toolchain::Toolchain>> toolchains;

    /// The information on each machine
    /// XXX: currently only handle host == build configurations, as we don't have
    /// a machine file
    Machines::PerMachine<Machines::Info> machines;
};

/**
 * Transitive state
 *
 * State that is valid only for a single run of meson++, and must be
 * regenerated each time we start meson
 */
class Transitive {
  public:
    Transitive() {};
    ~Transitive() {};
};

}
