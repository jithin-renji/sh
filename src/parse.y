%{

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define YYDEBUG     1

extern char *cur_cmd;
extern char *cur_ch;

int yylex(void);
void yyerror(char const *);

%}

%define api.value.type {char *}
%token ARG

%left '|'

%%
input:
    %empty
    | command_list
    | pipeline
    | pipeline ';' command_list
    ;

pipeline:
    simple_command '|' simple_command
    | pipeline '|' simple_command

command_list:
    simple_command
    | command_list ';' simple_command
    | command_list ';' pipeline
    | command_list ';'
    ;

simple_command:
    ARG
    | simple_command ARG
    ;

%%

int isreserved(char c)
{
    switch (c) {
    case '|':
    case '<':
    case '>':
    case '&':
    case ';':
        return 1;

    default:
    }

    return 0;
}

int yylex(void)
{
    while (*cur_ch == ' ' || *cur_ch == '\t') {
        cur_ch++;
    }

    if (*cur_ch && !isspace(*cur_ch) && !isreserved(*cur_ch)) {
        size_t n_char = 0;
        while (*cur_ch && !isspace(*cur_ch) && !isreserved(*cur_ch)) {
            n_char++;
            cur_ch++;
        }

        yylval = strndup(cur_ch - n_char, n_char + 1);
        yylval[n_char] = '\0';

        return ARG;
    }

    return *cur_ch++;
}

void yyerror(char const *e)
{
    fprintf(stderr, "%s\n", e);
}
