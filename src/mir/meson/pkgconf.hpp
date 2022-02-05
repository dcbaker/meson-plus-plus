// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#pragma once

#include <libpkgconf++/pkgconf++.hpp>

#include "state/state.hpp"

namespace MIR::Dependencies {

struct ErrorData {
    std::string message{};
};

/**
 * Find a pkgconfig dependency using libpkgconf
 *
 * Encapsulates the nmanual memory management of libpkgconf
 */
class PkgConf {
  public:
    PkgConf();

    /// Run libpkgconf and get the result
    State::Dependency operator()(const std::string & name, const std::string & version);

  private:
    pkgconfpp::Context context;
};

} // namespace MIR::Dependencies
