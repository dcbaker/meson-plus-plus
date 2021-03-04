// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

%skeleton "lalr1.cc"
%require  "3.2"
%debug
%defines
%define api.namespace { Frontend }
%define api.parser.class { Parser }

%code requires {
    #include <memory>
    #include "node.hpp"

    namespace Frontend {
        class Scanner;
    }

}

%parse-param { Scanner & scanner }
%parse-param { std::unique_ptr<AST::CodeBlock> & block }


%code {
    #include <iostream>
    #include <fstream>
    #include <memory>

    #include "node.hpp"
    #include "scanner.hpp"

    #undef yylex
    #define yylex scanner.yylex
}

%define api.value.type variant
%define parse.assert

%token <std::string>    IDENTIFIER STRING
%token <int64_t>        DECIMAL_NUMBER OCTAL_NUMBER HEX_NUMBER
%token <bool>           BOOL
%token                  EQUAL
%token                  END                 0

%nterm <std::unique_ptr<AST::Number>>          hex_literal decimal_literal octal_literal integer_literal
%nterm <std::unique_ptr<AST::Boolean>>         boolean_literal
%nterm <std::unique_ptr<AST::String>>          string_literal
%nterm <std::unique_ptr<AST::Identifier>>      identifier_expression
%nterm <std::unique_ptr<AST::Assignment>>      assignment_expression
%nterm <std::unique_ptr<AST::Expression>>      literal expression
%nterm <std::unique_ptr<AST::CodeBlock>>       program expressions

%%

program : expressions END              { block = std::move($1); }
        ;

expressions : expression                { $$ = std::make_unique<AST::CodeBlock>($1); }
            | expressions expression    { $1->expressions.push_back(std::move($2)); $$ = std::move($1); }

expression : literal                { $$ = std::move($1); }
           | identifier_expression  { $$ = std::move($1); }
           | assignment_expression  { $$ = std::move($1); }
           ;

assignment_expression : identifier_expression EQUAL expression { $$ = std::make_unique<AST::Assignment>(*$1, $3); }
                      ;

literal : integer_literal  { $$ = std::move($1); }
        | string_literal   { $$ = std::move($1); }
        | boolean_literal  { $$ = std::move($1); }
        ;

boolean_literal : BOOL { $$ = std::make_unique<AST::Boolean>($1); }
                ;

string_literal : STRING { $$ = std::make_unique<AST::String>($1.substr(1, $1.size() - 2)); }
               ;

integer_literal : hex_literal       { $$ = std::move($1); }
                | decimal_literal   { $$ = std::move($1); }
                | octal_literal     { $$ = std::move($1); }
                ;

hex_literal : HEX_NUMBER { $$ = std::make_unique<AST::Number>($1); }
            ;

decimal_literal : DECIMAL_NUMBER { $$ = std::make_unique<AST::Number>($1); }
                ;

octal_literal : OCTAL_NUMBER { $$ = std::make_unique<AST::Number>($1); }
              ;

identifier_expression : IDENTIFIER { $$ = std::make_unique<AST::Identifier>($1); }
                      ;

%%

void Frontend::Parser::error(const std::string & err_message)
{
   std::cerr << "Error: " << err_message << std::endl;
}
