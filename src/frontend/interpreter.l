%option noyywrap

%%

[0-9]+       { printf("number: %s\n", yytext); }
[a-zA-Z0-9]+ { printf("string: %s\n", yytext); }

%%

int main() {
    yyFlexLexer lexer{};
    while(lexer.yylex() != 0)
        ;
    return 0;
}