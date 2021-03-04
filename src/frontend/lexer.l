/* SPDX-license-identifier: Apache-2.0 */
/* Copyright © 2021 Intel Corporation */

%{
#include <string>

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
[a-zA-Z_][a-zA-Z0-9_]*          { lval->build<std::string>(yytext); return token::IDENTIFIER; }
.                               { yyterminate(); }

%%
