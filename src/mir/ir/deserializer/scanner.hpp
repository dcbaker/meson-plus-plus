// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <cassert>
#include <string>

#ifndef yyFlexLexerOnce
#include <FlexLexer.h>
#endif

#include "deserialize.yy.hpp"

namespace MIR::IR::Serial {

class Scanner : public yyFlexLexer {
  public:
    Scanner(std::istream * in, std::string_view s) : yyFlexLexer{in}, filename{s} {};
    ~Scanner() override = default;

    using FlexLexer::yylex;

    virtual int yylex(Parser::semantic_type * const lval, Parser::location_type * loc);

    std::string filename;
};

}; // namespace MIR::IR::Serial
