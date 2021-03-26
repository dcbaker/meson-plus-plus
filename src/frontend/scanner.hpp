// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cassert>
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
  private:
    /**
     * Increase the brace level by one
     */
    void inc_brace() { inside_brace += 1; }

    /**
     * Decrease the brace level by one
     */
    void dec_brace() {
        assert(inside_brace > 0);
        inside_brace -= 1;
    }

    /**
     * Are we inside a brace?
     */
    bool brace() { return inside_brace > 0; }

    /**
     * Track if we're inside a brace, and how deep
     *
     * We need to use a counter rather than a bool as it's perfectly valid (and
     * quite common) to have multiple levels of depth, for example
     * ```meson
     * add_project_arguments(
     *   cc.get_supported_arguments([
     *     'a',
     *     'b',
     *   ],
     *   language : 'c',
     * )
     *
     * has theree levels, one for `add_project_arguments`, one for
     * `cc.get_supported_arguments`, and one for the array literal
     */
    unsigned inside_brace = 0;
};

}; // namespace Frontend
