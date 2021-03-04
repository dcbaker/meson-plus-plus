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

%%

identifier : IDENTIFIER { std::cout << "identifier: " << $1 << std::endl; }
           ;

%%

void
Frontend::Parser::error(const std::string & err_message)
{
   std::cerr << "Error: " << err_message << std::endl;
}
