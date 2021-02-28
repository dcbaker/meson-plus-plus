/* SPDX-license-identifier: Apache-2.0 */
/* Copyright © 2021 Intel Corporation */

%option noyywrap
%option nodefault

%{
%}

%%

,                 { std::cout << "comma" << std::endl;}
[0-9]+            { std::cout << "number: " << yytext << std::endl; }
[a-zA-Z0-9_.+-]+  { std::cout << "string: " << yytext << std::endl; }

%%

int main() {
    yyFlexLexer lexer{};

    while(lexer.yylex() != 0) {
        ;
    }
    return 0;
}
