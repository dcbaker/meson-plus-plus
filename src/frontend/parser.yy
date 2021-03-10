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
%token                  ADD                 "+"
%token                  SUB                 "-"
%token                  MUL                 "*"
%token                  DIV                 "/"
%token                  MOD                 "%"
%token                  UMINUS
%token                  END                 0

%nterm <AST::ExpressionV>                       literal expression
%nterm <std::unique_ptr<AST::CodeBlock>>       program expressions

%left                   "-" "+"
%left                   "*" "/" "%"
%left                   "(" ")"
%right                  UMINUS      // Negation

%%

program : expressions END                           { block = std::move($1); }
        ;

expressions : expression                            { $$ = std::make_unique<AST::CodeBlock>(std::move($1)); }
            | expressions expression                { $1->expressions.push_back(std::move($2)); $$ = std::move($1); }

expression : expression "+" expression              { $$ = AST::ExpressionV(std::move(std::make_unique<AST::AdditiveExpression>(std::move($1), AST::AddOp::ADD, std::move($3)))); }
           | expression "-" expression              { $$ = AST::ExpressionV(std::move(std::make_unique<AST::AdditiveExpression>(std::move($1), AST::AddOp::SUB, std::move($3)))); }
           | expression "*" expression              { $$ = AST::ExpressionV(std::move(std::make_unique<AST::MultiplicativeExpression>(std::move($1), AST::MulOp::MUL, std::move($3)))); }
           | expression "/" expression              { $$ = AST::ExpressionV(std::move(std::make_unique<AST::MultiplicativeExpression>(std::move($1), AST::MulOp::DIV, std::move($3)))); }
           | expression "%" expression              { $$ = AST::ExpressionV(std::move(std::make_unique<AST::MultiplicativeExpression>(std::move($1), AST::MulOp::MOD, std::move($3)))); }
           | "(" expression ")"                     { $$ = std::move($2); }
           | "-" expression %prec UMINUS            { $$ = AST::ExpressionV(std::move(std::make_unique<AST::UnaryExpression>(AST::UnaryOp::NEG, std::move($2)))); }
           | literal                                { $$ = std::move($1); }
           | IDENTIFIER                             { $$ = AST::ExpressionV(std::move(std::make_unique<AST::Identifier>($1))); }
           ;

literal : HEX_NUMBER                                { $$ = AST::ExpressionV(std::move(std::make_unique<AST::Number>($1))); }
        | DECIMAL_NUMBER                            { $$ = AST::ExpressionV(std::move(std::make_unique<AST::Number>($1))); }
        | OCTAL_NUMBER                              { $$ = AST::ExpressionV(std::move(std::make_unique<AST::Number>($1))); }
        | STRING                                    { $$ = AST::ExpressionV(std::move(std::make_unique<AST::String>($1.substr(1, $1.size() - 2)))); }
        | BOOL                                      { $$ = AST::ExpressionV(std::move(std::make_unique<AST::Boolean>($1))); }
        ;

%%

void Frontend::Parser::error(const location_type & loc, const std::string & err_message)
{
   std::cerr << "Error: " << err_message << " at " << loc << std::endl;
}
