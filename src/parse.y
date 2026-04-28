%{

#include "vec.h"
#include "cmd_ast.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define YYDEBUG     1

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

%token ERROR
%token <word> WORD
%type <words> simple_command
%type <ast_root> async_command command_list pipeline input

%left '|'
%precedence ';'

%%
input:
    %empty {
        *root = NULL;
    }
    | command_list {
        *root = $1;
    }
    | pipeline {
        *root = $1;
    }
    | async_command {
        *root = $1;
    }
    | pipeline ';' command_list {
        *root = ast_node_create(COMMAND_LIST, NULL, NULL, NULL, 0, $1, $3);
    }
    ;

pipeline:
    simple_command '|' simple_command {
        $$ = ast_node_create(
            PIPELINE,
            NULL,
            NULL, NULL, 0,
            ast_node_create(SIMPLE_COMMAND, $1, NULL, NULL, 0, NULL, NULL),
            ast_node_create(SIMPLE_COMMAND, $3, NULL, NULL, 0, NULL, NULL)
        );
    }
    | pipeline '|' simple_command {
        $$ = ast_node_create(
            PIPELINE,
            NULL,
            NULL, NULL, 0,
            $1,
            ast_node_create(SIMPLE_COMMAND, $3, NULL, NULL, 0, NULL, NULL)
        );
    }
    ;

command_list:
    simple_command {
        $$ = ast_node_create(SIMPLE_COMMAND, $1, NULL, NULL, 0, NULL, NULL);
    }
    | simple_command '>' WORD {
        printf("File redirection detected\n");
        $$ = ast_node_create(SIMPLE_COMMAND, $1, NULL, $3, 0, NULL, NULL);
    }
    | command_list ';' simple_command {
        $$ = ast_node_create(
            COMMAND_LIST,
            NULL,
            NULL, NULL, 0,
            $1,
            ast_node_create(SIMPLE_COMMAND, $3, NULL, NULL, 0, NULL, NULL)
        );
    }
    | command_list ';' simple_command '>' WORD {
        $$ = ast_node_create(
            COMMAND_LIST,
            NULL,
            NULL, NULL, 0,
            $1,
            ast_node_create(SIMPLE_COMMAND, $3, NULL, $5, 0, NULL, NULL)
        );
    }
    | command_list ';' pipeline {
        $$ = ast_node_create(COMMAND_LIST, NULL, NULL, NULL, 0, $1, $3);
    }
    | pipeline ';'
    | command_list ';'
    ;

simple_command:
    WORD {
        $$ = vec_create();
        $$ = vec_append($$, $1);
    }
    | ERROR { YYABORT; }
    | simple_command WORD {
        $$ = vec_append($1, $2);
    }
    ;

async_command:
    simple_command '&' {
        $$ = ast_node_create(
            ASYNC_COMMAND, NULL,
            NULL, NULL, 0,
            ast_node_create(SIMPLE_COMMAND, $1, NULL, NULL, 0, NULL, NULL), NULL
        ); 
    }
    | pipeline '&' {
        $$ = ast_node_create(ASYNC_COMMAND, NULL, NULL, NULL, 0, $1, NULL);
    }

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
        break;
    }

    return 0;
}

int yylex(void)
{
    while (*cur_ch == ' ' || *cur_ch == '\t') {
        cur_ch++;
    }

    /* Is there a better way to deal with quoting? */
    int quoted = 0;
    int quote_ended = 0;
    if (*cur_ch && !isspace(*cur_ch) && !isreserved(*cur_ch)) {
        if (*cur_ch == '"') {
            cur_ch++;
            quoted = 1;
        }

        size_t n_char = 0;
        while (*cur_ch && (quoted || (!isspace(*cur_ch) && !isreserved(*cur_ch)))) {
            if (*cur_ch == '"') {
                quoted = 0;
                quote_ended = 1;
                cur_ch++;
                break;
            }

            cur_ch++;
            n_char++;
        }

        if (quoted && !quote_ended) {
            return ERROR;
        }

        if (quote_ended) {
            yylval.word = strndup(cur_ch - n_char - 1, n_char);
        } else {
            yylval.word = strndup(cur_ch - n_char, n_char + 1);
        }
        yylval.word[n_char] = '\0';

        return WORD;
    }

    return *cur_ch++;
}

void yyerror(ASTNode_t **root, char const *e)
{
    fprintf(stderr, "%s\n", e);
}
