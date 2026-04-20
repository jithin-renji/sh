%{

#include <stdio.h>
int yylex(void);
void yyerror(char const *);

%}

%define api.value.type {double}
%token NUM

%%
input:
    %empty
    ;

%%

int yylex(void)
{
    return 0;
}

void yyerror(char const *e)
{

}
