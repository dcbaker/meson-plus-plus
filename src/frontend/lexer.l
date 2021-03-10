/* SPDX-license-identifier: Apache-2.0 */
/* Copyright © 2021 Intel Corporation */

%{
#include <cstdint>
#include <string>
#include <iostream>

#include "scanner.hpp"

#undef  YY_DECL
#define YY_DECL int Frontend::Scanner::yylex(Frontend::Parser::semantic_type * lval, Frontend::Parser::location_type * loc)

/* typedef to make the returns for the tokens shorter */
using token = Frontend::Parser::token;

/* For windows */
#define YY_NO_UNISTD_H

#define YY_USER_ACTION loc->step(); loc->columns(yyleng);

%}

%option debug
%option nodefault
%option yyclass="Frontend::Scanner"
%option noyywrap
%option c++

%%


[ \t]                           { }
\n                              { loc->lines(); }
(true|false)                    { lval->build<bool>(std::string(yytext) == "true"); return token::BOOL; }
[a-zA-Z_][a-zA-Z0-9_]*          { lval->build<std::string>(yytext); return token::IDENTIFIER; }
'.*'                            { lval->build<std::string>(yytext); return token::STRING; }
0x[0-9a-fA-F]+                  { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 16)); return token::HEX_NUMBER; }
0o[0-7]+                        { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 8)); return token::OCTAL_NUMBER; }
[0-9]+                          { lval->build<int64_t>(std::stoll(yytext)); return token::DECIMAL_NUMBER; }
(>|>=|==|!=|=<|<)               { lval->build<std::string>(yytext); return token::RELATIONAL; }
\-                              { return token::SUB; }
\+                              { return token::ADD; }
\*                              { return token::MUL; }
\/                              { return token::DIV; }
\%                              { return token::MOD; }
=                               { return token::EQUAL; }
\[                              { return token::LBRACKET; }
\]                              { return token::RBRACKET; }
\(                              { return token::LPAREN; }
\)                              { return token::RPAREN; }
,                               { return token::COMMA; }
:                               { return token::COLON; }
\.                              { return token::DOT; }
<<EOF>>                         { return token::END; }
\s*#.*$                         { }
.                               { }

%%
