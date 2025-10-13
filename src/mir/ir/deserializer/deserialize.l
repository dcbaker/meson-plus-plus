/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright © 2025 Intel Corporation */

%{

#include "ir/deserializer/scanner.hpp"
#include "ir/deserializer/serial.hpp"

#include "deserialize.yy.hpp"

#include <cstdint>
#include <string>
#include <iostream>

#undef  YY_DECL
#define YY_DECL int MIR::IR::Serial::Scanner::yylex( \
    MIR::IR::Serial::Parser::semantic_type * lval,   \
    MIR::IR::Serial::Parser::location_type * loc)

/* typedef to make the returns for the tokens shorter */
using token = MIR::IR::Serial::Parser::token;

/* For windows */
#define YY_NO_UNISTD_H

#define YY_USER_ACTION loc->step(); loc->columns(yyleng);

%}

%option debug
%option nodefault
%option yyclass="MIR::IR::Serial::Scanner"
%option noyywrap
%option c++

%%

(true|false)                    {
                                    lval->build<bool>(std::string(yytext) == "true");
                                    return token::BOOL;
                                }
[0-9]+                          {
                                    lval->build<uint64_t>(std::stoll(yytext));
                                    return token::NUMBER;
                                }
\"[^"]*\"                       {
                                    lval->build<std::string>(yytext);
                                    return token::STRING;
                                }
\{                              { return token::LCURLY; }
\}                              { return token::RCURLY; }
\=                              { return token::EQUAL; }
.                               { yyterminate(); }

%%
