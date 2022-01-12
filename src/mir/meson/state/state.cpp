// SPDX-license-identifier: Apache-2.0
// Copyright © 2021-2022 Dylan Baker

#include "state.hpp"

namespace MIR::State {

Persistant::Persistant(const std::filesystem::path & sr_, const std::filesystem::path & br_)
    : toolchains{}, machines{Machines::detect_build()}, source_root{sr_},
      build_root{br_}, programs{} {};

}
