/* SPDX-license-identifier: Apache-2.0 */
/* Copyright © 2021 Intel Corporation */

%{
#include <cstdint>
#include <string>
#include <iostream>

#include "scanner.hpp"
#undef  YY_DECL
#define YY_DECL int Frontend::Scanner::yylex(Frontend::Parser::semantic_type * lval)

/* typedef to make the returns for the tokens shorter */
using token = Frontend::Parser::token;

/* msvc2010 requires that we exclude this header file. */
#define YY_NO_UNISTD_H

%}

%option debug
%option nodefault
%option yyclass="Frontend::Scanner"
%option noyywrap
%option c++

%%


[ \t\n]                         { }
(true|false)                    { lval->build<bool>(std::string(yytext) == "true"); return token::BOOL; }
[a-zA-Z_][a-zA-Z0-9_]*          { lval->build<std::string>(yytext); return token::IDENTIFIER; }
'.*'                            { lval->build<std::string>(yytext); return token::STRING; }
0x[0-9a-zA-Z]+                  { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 16)); return token::HEX_NUMBER; }
0o[0-7]+                        { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 8)); return token::OCTAL_NUMBER; }
[0-9]+                          { lval->build<int64_t>(std::stoll(yytext)); return token::DECIMAL_NUMBER; }
.                               { std::cerr << "unknown statement: " << yytext << std::endl; }

%%
