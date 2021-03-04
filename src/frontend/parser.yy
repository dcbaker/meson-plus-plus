// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

%skeleton "lalr1.cc"
%require  "3.2"
%debug
%defines
%define api.namespace { Frontend }
%define api.parser.class { Parser }

%code requires {

   namespace Frontend {
      class Scanner;
   }

}

%parse-param { Scanner  &scanner }

%code{
   #include <iostream>
   #include <fstream>

   #include "scanner.hpp"

#undef yylex
#define yylex scanner.yylex
}

%define api.value.type variant
%define parse.assert

%token <std::string>    IDENTIFIER
%token <std::string>    STRING
%token <int64_t>        DECIMAL_NUMBER
%token <int64_t>        OCTAL_NUMBER
%token <int64_t>        HEX_NUMBER
%token <bool>           BOOL
%token                  END                 0

%%

program : literal
        | program literal
        ;

literal : integer_literal
        | string_literal
        | boolean_literal
        | identifier
        ;

boolean_literal : BOOL { std::cout << "bool: " << $1 << std::endl; }
                ;

string_literal : STRING { std::cout << "string: " << $1 << std::endl; }
               ;

integer_literal : HEX_NUMBER { std::cout << "hex number: " << $1 << std::endl; }
                | DECIMAL_NUMBER { std::cout << "decimal number: " << $1 << std::endl; }
                | OCTAL_NUMBER { std::cout << "octal number: " << $1 << std::endl; }
                ;

identifier : IDENTIFIER { std::cout << "identifier: " << $1 << std::endl; }
           ;

%%

void
Frontend::Parser::error(const std::string & err_message)
{
   std::cerr << "Error: " << err_message << std::endl;
}
