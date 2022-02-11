// SPDX-license-identifier: Apache-2.0
// Copyright © 2021-2022 Dylan Baker

#include "state.hpp"

namespace MIR::State {

Persistant::Persistant(std::filesystem::path sr_, std::filesystem::path br_)
    : toolchains{}, machines{Machines::detect_build()}, source_root{std::move(sr_)},
      build_root{std::move(br_)}, programs{} {};
}
