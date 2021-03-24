// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <string>

#ifndef yyFlexLexerOnce
#include <FlexLexer.h>
#endif

#include "parser.yy.hpp"

namespace Frontend {

class Scanner : public yyFlexLexer {
  public:
    Scanner(std::istream * in, const std::string & s) : yyFlexLexer{in}, filename{s} {};
    ~Scanner(){};

    using FlexLexer::yylex;

    virtual int yylex(Frontend::Parser::semantic_type * const lval, Frontend::Parser::location_type * loc);

    const std::string filename;
};

}; // namespace Frontend
