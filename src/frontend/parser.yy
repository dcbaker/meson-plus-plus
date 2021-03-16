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
%token <std::string>    RELATIONAL
%token                  LBRACKET            "["
%token                  RBRACKET            "]"
%token                  LCURLY              "{"
%token                  RCURLY              "}"
%token                  LPAREN              "("
%token                  RPAREN              ")"
%token                  ADD                 "+"
%token                  SUB                 "-"
%token                  MUL                 "*"
%token                  DIV                 "/"
%token                  MOD                 "%"
%token                  EQUAL               "="
%token                  COMMA               ","
%token                  COLON               ":"
%token                  DOT                 "."
%token                  UMINUS NEWLINE
%token                  END                 0

%nterm <AST::ExpressionV>                           literal expression
%nterm <AST::StatementV>                            statement
%nterm <AST::KeywordPair>                           keyword_item
%nterm <AST::KeywordList>                           keyword_arguments
%nterm <std::unique_ptr<AST::Arguments>>            arguments
%nterm <AST::ExpressionList>                        positional_arguments
%nterm <std::unique_ptr<AST::CodeBlock>>            program statements

%left                   "-" "+"
%left                   "*" "/" "%"
%left                   "(" ")"
%left                   RELATIONAL
%right                  UMINUS      // Negation

%%

program : statements END                           { block = std::move($1); }
        ;

statements : statement                              { $$ = std::make_unique<AST::CodeBlock>(std::move($1)); }
           | statements NEWLINE statement           { $1->statements.push_back(std::move($3)); $$ = std::move($1); }
           ;

statement : expression                              { $$ = AST::StatementV(std::make_unique<AST::Statement>(std::move($1))); }
          ;

expression : expression "+" expression              { $$ = AST::ExpressionV(std::make_unique<AST::AdditiveExpression>(std::move($1), AST::AddOp::ADD, std::move($3))); }
           | expression "-" expression              { $$ = AST::ExpressionV(std::make_unique<AST::AdditiveExpression>(std::move($1), AST::AddOp::SUB, std::move($3))); }
           | expression "*" expression              { $$ = AST::ExpressionV(std::make_unique<AST::MultiplicativeExpression>(std::move($1), AST::MulOp::MUL, std::move($3))); }
           | expression "/" expression              { $$ = AST::ExpressionV(std::make_unique<AST::MultiplicativeExpression>(std::move($1), AST::MulOp::DIV, std::move($3))); }
           | expression "%" expression              { $$ = AST::ExpressionV(std::make_unique<AST::MultiplicativeExpression>(std::move($1), AST::MulOp::MOD, std::move($3))); }
           | expression "=" expression              { $$ = AST::ExpressionV(std::make_unique<AST::Assignment>(std::move($1), std::move($3))); }
           | expression "[" expression "]"          { $$ = AST::ExpressionV(std::make_unique<AST::Subscript>(std::move($1), std::move($3))); }
           | expression RELATIONAL expression       { $$ = AST::ExpressionV(std::make_unique<AST::Relational>(std::move($1), $2, std::move($3))); } // XXX: this might now actually be safe, since x < y < z isnt valid.
           | "(" expression ")"                     { $$ = std::move($2); } // XXX: Do we need a subexpression type?
           | "-" expression %prec UMINUS            { $$ = AST::ExpressionV(std::make_unique<AST::UnaryExpression>(AST::UnaryOp::NEG, std::move($2))); }
           | expression "(" arguments ")"           { $$ = AST::ExpressionV(std::make_unique<AST::FunctionCall>(std::move($1), std::move($3))); }
           | expression "." expression "(" arguments ")" { $$ = AST::ExpressionV(std::make_unique<AST::MethodCall>(std::move($1), std::move($3), std::move($5))); }
           | "[" positional_arguments "]"           { $$ = AST::ExpressionV(std::make_unique<AST::Array>(std::move($2))); }
           | "[" "]"                                { $$ = AST::ExpressionV(std::make_unique<AST::Array>()); }
           | "{" keyword_arguments "}"              { $$ = AST::ExpressionV(std::make_unique<AST::Dict>(std::move($2))); }
           | "{" "}"                                { $$ = AST::ExpressionV(std::make_unique<AST::Dict>()); }
           | literal                                { $$ = std::move($1); }
           | IDENTIFIER                             { $$ = AST::ExpressionV(std::make_unique<AST::Identifier>($1)); }
           ;

arguments : %empty                                  { $$ = std::make_unique<AST::Arguments>(); }
          | positional_arguments                    { $$ = std::make_unique<AST::Arguments>(std::move($1)); }
          | keyword_arguments                       { $$ = std::make_unique<AST::Arguments>(std::move($1)); }
          | positional_arguments "," keyword_arguments { $$ = std::make_unique<AST::Arguments>(std::move($1), std::move($3)); }
          ;

positional_arguments : expression                   { $$ = AST::ExpressionList(); $$.emplace_back(std::move($1)); }
                     | positional_arguments "," expression { $1.emplace_back(std::move($3)); $$ = std::move($1); }
                     ;

keyword_arguments : keyword_item                    { $$ = AST::KeywordList(); $$.emplace_back(std::move($1)); }
                  | keyword_arguments "," keyword_item { $1.emplace_back(std::move($3)); $$ = std::move($1); }
                  ;

keyword_item : expression ":" expression            { $$ = AST::KeywordPair(std::move($1), std::move($3)); }
             ;

literal : HEX_NUMBER                                { $$ = AST::ExpressionV(std::make_unique<AST::Number>($1)); }
        | DECIMAL_NUMBER                            { $$ = AST::ExpressionV(std::make_unique<AST::Number>($1)); }
        | OCTAL_NUMBER                              { $$ = AST::ExpressionV(std::make_unique<AST::Number>($1)); }
        | STRING                                    { $$ = AST::ExpressionV(std::make_unique<AST::String>($1.substr(1, $1.size() - 2))); }
        | BOOL                                      { $$ = AST::ExpressionV(std::make_unique<AST::Boolean>($1)); }
        ;

%%

void Frontend::Parser::error(const location_type & loc, const std::string & err_message)
{
   std::cerr << "Error: " << err_message << " at " << loc << std::endl;
}
