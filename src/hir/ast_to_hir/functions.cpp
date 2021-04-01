// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "functions.hpp"

namespace HIR::FromAST::Functions {

void project(State::Persistant & pers, State::Transitive & trans, Frontend::AST::FunctionCall & func) {
    auto & pos = func.args->positional;
    auto & kw = func.args->keyword;
};

}

