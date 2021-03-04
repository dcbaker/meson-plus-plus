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

%parse-param { Scanner  &scanner }

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

%token <std::string>    IDENTIFIER
%token <std::string>    STRING
%token <int64_t>        DECIMAL_NUMBER OCTAL_NUMBER HEX_NUMBER
%token <bool>           BOOL
%token                  END                 0

%nterm <std::unique_ptr<AST::Number>>          hex_literal
%nterm <std::unique_ptr<AST::Number>>          decimal_literal
%nterm <std::unique_ptr<AST::Number>>          octal_literal
%nterm <std::unique_ptr<AST::Boolean>>         boolean_literal

%%

program : expression
        | program expression
        ;

expression : literal
           | identifier_expression
           | subscript_expression
           ;

subscript_expression : expression "[" expression "]"
                     ;

literal : integer_literal
        | string_literal
        | boolean_literal
        ;

boolean_literal : BOOL { $$ = std::make_unique<AST::Boolean>($1); }
                ;

string_literal : STRING
               ;

integer_literal : hex_literal
                | decimal_literal
                | octal_literal
                ;

hex_literal : HEX_NUMBER { $$ = std::make_unique<AST::Number>($1); }
            ;

decimal_literal : DECIMAL_NUMBER { $$ = std::make_unique<AST::Number>($1); }
                ;

octal_literal : OCTAL_NUMBER { $$ = std::make_unique<AST::Number>($1); }
              ;

identifier_expression : IDENTIFIER
                      ;

%%

void Frontend::Parser::error(const std::string & err_message)
{
   std::cerr << "Error: " << err_message << std::endl;
}
