// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

%skeleton "lalr1.cc"
%require  "3.2"
%debug
%locations
%defines
%define api.namespace { Frontend }
%define api.parser.class { Parser }
%define api.location.file "locations.hpp"

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
%token                  EQUAL LBRACKET RBRACKET LPAREN RPAREN
%token                  NOT PLUS MINUS DIV MUL MOD
%token                  END                 0

%left                   LPAREN

%nterm <std::unique_ptr<AST::Number>>          hex_literal decimal_literal octal_literal integer_literal
%nterm <std::unique_ptr<AST::Boolean>>         boolean_literal
%nterm <std::unique_ptr<AST::String>>          string_literal
%nterm <std::unique_ptr<AST::Identifier>>      identifier_expression
%nterm <std::unique_ptr<AST::Expression>>      literal primary_expression postfix_expression
%nterm <std::unique_ptr<AST::Expression>>      unary_expression multiplicative_expression
%nterm <std::unique_ptr<AST::CodeBlock>>       program expressions
%nterm <AST::UnaryOpEnum>                      unary_operator
%nterm <AST::MulOpEnum>                        multiplicative_operator

%%

program : expressions END                           { block = std::move($1); }
        ;

expressions : multiplicative_expression             { $$ = std::make_unique<AST::CodeBlock>($1); }
            | expressions multiplicative_expression { $1->expressions.push_back(std::move($2)); $$ = std::move($1); }

multiplicative_expression : unary_expression        { $$ = std::move($1); }
                          | multiplicative_expression multiplicative_operator multiplicative_expression { $$ = std::make_unique<AST::MultiplicativeExpression>(std::move($1), $2, std::move($3)); }
                          ;

multiplicative_operator : MUL                       { $$ = AST::MulOpEnum::MUL; }
                        | DIV                       { $$ = AST::MulOpEnum::DIV; }
                        | MOD                       { $$ = AST::MulOpEnum::MOD; }
                        ;

unary_expression : postfix_expression               { $$ = std::move($1); }
                 | unary_operator unary_expression  { $$ = std::make_unique<AST::UnaryExpression>($1, std::move($2)); }
                 ;

unary_operator : PLUS                               { $$ = AST::UnaryOpEnum::PLUS; }
               | MINUS                              { $$ = AST::UnaryOpEnum::MINUS; }
               | NOT                                { $$ = AST::UnaryOpEnum::NOT; }
               ;

postfix_expression : primary_expression             { $$ = std::move($1); }
                   ;

primary_expression : literal                        { $$ = std::move($1); }
                   | identifier_expression          { $$ = std::move($1); }
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

void Frontend::Parser::error(const location_type & loc, const std::string & err_message)
{
   std::cerr << "Error: " << err_message << " at " << loc << std::endl;
}
