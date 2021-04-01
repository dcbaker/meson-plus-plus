// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#pragma once

#include "node.hpp"
#include "state/state.hpp"

namespace HIR::FromAST::Functions {

/**
 * Handle the meson `project()` function
 */
void project(State::Persistant &, State::Transitive &, Frontend::AST::FunctionCall &);

}
