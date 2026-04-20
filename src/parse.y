%{

#include "vec.h"
#include "cmd_ast.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define YYDEBUG     1

extern char *cur_cmd;
extern char *cur_ch;

int yylex(void);
void yyerror(ASTNode_t **root, char const *e);

%}

%parse-param { ASTNode_t **root }

%union {
    char *word;
    Vec_t *words;
    ASTNode_t *ast_root;
}

%token <word> WORD
%type <words> simple_command
%type <ast_root> command_list
%type <ast_root> pipeline

%left '|'

%%
input:
    %empty
    | command_list
    | pipeline
    | pipeline ';' command_list
    ;

pipeline:
    simple_command '|' simple_command {
        *root = ast_node_create(PIPELINE, NULL,
                    ast_node_create(SIMPLE_COMMAND, $1, NULL, NULL),
                    ast_node_create(SIMPLE_COMMAND, $3, NULL, NULL));

        $$ = *root;
    }
    | pipeline '|' simple_command {
        ASTNode_t *new_node = ast_node_create(PIPELINE, NULL,
                                $1, ast_node_create(SIMPLE_COMMAND, $3, NULL, NULL));
        new_node->left = $1;
        *root = new_node;
        $$ = new_node;
    }

command_list:
    simple_command {
        *root = ast_node_create(SIMPLE_COMMAND, $1, NULL, NULL);
        $$ = *root;
    }
    | command_list ';' simple_command {
        ASTNode_t *new_command = ast_node_create(SIMPLE_COMMAND, $3, NULL, NULL);
        $1->right = new_command;
        $$ = new_command;
    }
    | command_list ';' pipeline
    | command_list ';'
    ;

simple_command:
    WORD {
        $$ = vec_create();
        $$ = vec_append($$, $1);
    }
    | simple_command WORD {
        $$ = vec_append($1, $2);
    }
    ;

%%

static int isreserved(char c)
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

        yylval.word = strndup(cur_ch - n_char, n_char + 1);
        yylval.word[n_char] = '\0';

        return WORD;
    }

    return *cur_ch++;
}

void yyerror(ASTNode_t **root, char const *e)
{
    fprintf(stderr, "%s\n", e);
}
