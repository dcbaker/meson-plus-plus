/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright © 2025 Intel Corporation
 */

%skeleton "lalr1.cc"
%require  "3.2"
%debug
%defines
%define api.namespace { MIR::IR::Serial }
%define api.parser.class { Parser }
%define api.location.file "deserialize_loc.hpp"

%code requires {
    namespace MIR::IR::Serial {
        class Scanner;
    }
}

%parse-param { Scanner & scanner }
// %parse-param { std::unique_ptr<AST::CodeBlock> & block }

%locations
%initial-action {
    @$.begin.filename = @$.end.filename = &scanner.filename;
}

%code {
    #include "scanner.hpp"
    #include "serial.hpp"

    #include <iostream>
    #include <fstream>
    #include <memory>

    #undef yylex
    #define yylex scanner.yylex
}

%define api.value.type variant
%define parse.assert

%token <std::string>    IDENTIFIER STRING
%token <int64_t>        NUMBER
%token <bool>           BOOL
%token                  EQUAL               "="
%token                  LCURLY              "{"
%token                  RCURLY              "}"

// %nterm <AST::ExpressionV>                           literal expression

%left                   EQUAL
%left                   LCURLY RCURLY

%%

program : %empty                                    { }
        ;

%%

void MIR::IR::Serial::Parser::error(const location_type & loc, const std::string & err_message)
{
   std::cerr << "Error: " << err_message << " at " << loc << std::endl;
}
