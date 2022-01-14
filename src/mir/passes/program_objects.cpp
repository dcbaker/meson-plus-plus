// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include "passes.hpp"

namespace MIR::Passes {

namespace {

std::optional<Object> lower_found_method(const Object & obj) { return std::nullopt; }

} // namespace

bool lower_program_objects(BasicBlock &, State::Persistant & pstate) {
    bool progress = false;

    return progress;
}

} // namespace MIR::Passes
