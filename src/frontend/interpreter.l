/* SPDX-license-identifier: Apache-2.0 */
/* Copyright © 2021 Intel Corporation */

%option noyywrap
%option nodefault

%{
    #include <string>
    #include <iostream>

    #include "node.hpp"
    #include "interpreter.tab.hpp"

    extern int yylex();

    #define SAVE_TOKEN yylval.string = new std::string{yytext, static_cast<std::string::size_type>(yyleng)}
    #define TOKEN(t) (yylval.token = t)
%}

%%

[ \t\n]                    ;  // Ignore whitespace
[a-zA-Z_][a-zA-Z0-9_]*     { SAVE_TOKEN; return TIDENTIFIER; }
[0-9]+                     { SAVE_TOKEN; return TINTEGER; }
.                          { std::cerr << "Uknown token: " << yytext << std::endl; yyterminate(); }

%%
