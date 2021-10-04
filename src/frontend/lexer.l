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

static std::string strbuffer{};

%}

%option debug
%option nodefault
%option yyclass="Frontend::Scanner"
%option noyywrap
%option c++

%x STRING_STATE
%x FSTRING_STATE
%x TSTRING_STATE
%%

    /* This is gross, but this is more working around how annoying newlines are in meson */
^[ \t]*"#".*\n?                 { loc->lines(); }
[ \t]+                          { }
^\s*\n                          {
                                    // We don't want to return a token, as there was no statement
                                    loc->lines();
                                }
"#".*                           { }
\\\n                            { loc-> lines(); }
\n                              {
                                    // Newlines are only significant as
                                    // statement separators We want to ignore
                                    // them inside of braces, as only
                                    // expressions and not statements are
                                    // allowed.
                                    loc->lines();
                                    if (!brace()) { return token::NEWLINE; }
                                }
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
[']{3}                          {
                                    BEGIN(TSTRING_STATE);
                                    strbuffer.clear();
                                    strbuffer.append(yytext);
                                }
\'                              {
                                    BEGIN(STRING_STATE);
                                    strbuffer.clear();
                                    strbuffer.append(yytext);
                                }
f'                              {
                                    BEGIN(FSTRING_STATE);
                                    strbuffer.clear();
                                    strbuffer.append(yytext);
                                }
<STRING_STATE,FSTRING_STATE,TSTRING_STATE>\\'   { strbuffer.append("'"); }
<STRING_STATE,FSTRING_STATE,TSTRING_STATE>\\n   { strbuffer.append("\n"); }
<STRING_STATE,FSTRING_STATE,TSTRING_STATE>\\t   { strbuffer.append("\t"); }
<STRING_STATE,FSTRING_STATE,TSTRING_STATE>\\\\  { strbuffer.append("\\"); }
<STRING_STATE,FSTRING_STATE>[^']  { strbuffer.append(yytext); }
<TSTRING_STATE>[^''']           { strbuffer.append(yytext); }
<STRING_STATE>\'                {
                                    strbuffer.append(yytext);
                                    lval->build<std::string>(strbuffer);
                                    BEGIN(INITIAL);
                                    return token::STRING;
                                }
<FSTRING_STATE>\'               {
                                    strbuffer.append(yytext);
                                    lval->build<std::string>(strbuffer);
                                    BEGIN(INITIAL);
                                    return token::FSTRING;
                                }
<TSTRING_STATE>[']{3}           {
                                    strbuffer.append(yytext);
                                    lval->build<std::string>(strbuffer);
                                    BEGIN(INITIAL);
                                    return token::TSTRING;
                                }
0[xX][0-9a-fA-F]+               { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 16)); return token::NUMBER; }
0[oO][0-7]+                     { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 8)); return token::NUMBER; }
0[bB][0-1]+                     { lval->build<int64_t>(std::stoll(std::string{yytext}.substr(2), nullptr, 2)); return token::NUMBER; }
[0-9]+                          { lval->build<int64_t>(std::stoll(yytext)); return token::NUMBER; }
\-                              { return token::SUB; }
\+                              { return token::ADD; }
\*                              { return token::MUL; }
\/                              { return token::DIV; }
\%                              { return token::MOD; }
(=|\+=|-=|\*=|\/=|\%=)          { lval->build<AST::AssignOp>(from_str(yytext)); return token::ASSIGN; }
\[                              { inc_brace(); return token::LBRACKET; }
\]                              { dec_brace(); return token::RBRACKET; }
\(                              { inc_brace(); return token::LPAREN; }
\)                              { dec_brace(); return token::RPAREN; }
\{                              { inc_brace(); return token::LCURLY; }
\}                              { dec_brace(); return token::RCURLY; }
,                               { return token::COMMA; }
:                               { return token::COLON; }
\?                              { return token::QMARK; }
\.                              { return token::DOT; }
.                               { yyterminate(); }

%%
