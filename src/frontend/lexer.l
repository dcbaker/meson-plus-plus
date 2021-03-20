/* SPDX-license-identifier: Apache-2.0 */
/* Copyright © 2021 Intel Corporation */

%{
#include <cstdint>
#include <string>
#include <iostream>

#include "scanner.hpp"
#include "node.hpp"

#undef  YY_DECL
#define YY_DECL int Frontend::Scanner::yylex(Frontend::Parser::semantic_type * lval, Frontend::Parser::location_type * loc)

/* typedef to make the returns for the tokens shorter */
using token = Frontend::Parser::token;

/* For windows */
#define YY_NO_UNISTD_H

#define YY_USER_ACTION loc->step(); loc->columns(yyleng);

Frontend::AST::AssignOp from_str(const std::string & s) {
    if (s == "=") {
        return Frontend::AST::AssignOp::EQUAL;
    } else if (s == "+=") {
        return Frontend::AST::AssignOp::ADD_EQUAL;
    } else if (s == "-=") {
        return Frontend::AST::AssignOp::SUB_EQUAL;
    } else if (s == "/=") {
        return Frontend::AST::AssignOp::DIV_EQUAL;
    } else if (s == "*=") {
        return Frontend::AST::AssignOp::MUL_EQUAL;
    } else if (s == "%=") {
        return Frontend::AST::AssignOp::MOD_EQUAL;
    } else {
        assert(false);
    }
}

%}

%option debug
%option nodefault
%option yyclass="Frontend::Scanner"
%option noyywrap
%option c++

%%

[ \t]                           { }
^\s*\n                          { loc->lines(); }
\n                              { loc->lines(); return token::NEWLINE; } // Newlines are only significant as statement separators
(true|false)                    { lval->build<bool>(std::string(yytext) == "true"); return token::BOOL; }
(>=|==|!=|<=|>|<|and|or|in|not\ in) { lval->build<std::string>(yytext); return token::RELATIONAL; }
not                             { return token::NOT; }
if                              { return token::IF; }
elif                            { return token::ELIF; }
else                            { return token::ELSE; }
endif                           { return token::ENDIF; }
foreach                         { return token::FOREACH; }
endforeach                      { return token::ENDFOREACH; }
break                           { return token::BREAK; }
continue                        { return token::CONTINUE; }
[a-zA-Z_][a-zA-Z0-9_]*          { lval->build<std::string>(yytext); return token::IDENTIFIER; }
'.*'                            { lval->build<std::string>(yytext); return token::STRING; }
0x[0-9a-fA-F]+                  { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 16)); return token::HEX_NUMBER; }
0o[0-7]+                        { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 8)); return token::OCTAL_NUMBER; }
[0-9]+                          { lval->build<int64_t>(std::stoll(yytext)); return token::DECIMAL_NUMBER; }
\-                              { return token::SUB; }
\+                              { return token::ADD; }
\*                              { return token::MUL; }
\/                              { return token::DIV; }
\%                              { return token::MOD; }
(=|\+=|-=|\*=|\/=|\%=)          { lval->build<AST::AssignOp>(from_str(yytext)); return token::ASSIGN; }
\[                              { return token::LBRACKET; }
\]                              { return token::RBRACKET; }
\(                              { return token::LPAREN; }
\)                              { return token::RPAREN; }
\{                              { return token::LCURLY; }
\}                              { return token::RCURLY; }
,                               { return token::COMMA; }
:                               { return token::COLON; }
\?                              { return token::QMARK; }
\.                              { return token::DOT; }
<<EOF>>                         { return token::END; }
\s*#.*$                         { }
.                               { }

%%
