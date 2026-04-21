#include "vec.h"
#include "cmd_ast.h"
#include "eval.h"

#ifdef HAVE_CONFIG_H
#   include <config.h>
#else
#   error "Missing config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

extern int yydebug;
extern int yyparse(ASTNode_t **root);

char *cur_cmd;
char *cur_ch;

int main(int argc, const char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "--debug") == 0)
        yydebug = 1;

    using_history();

    char *cmd = NULL;
    while ((cmd = readline(PACKAGE_NAME "-" PACKAGE_VERSION "$ "))) {
        if (*cmd) {
            add_history(cmd);
        }

        cur_cmd = cmd;
        cur_ch = cur_cmd;

        ASTNode_t *root = NULL;
        if (yyparse(&root) != 1 && root) {
            eval(root);
        }

        ast_free(root);

        free(cmd);
    }

    return 0;
}
