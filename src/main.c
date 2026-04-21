#include "vec.h"
#include "cmd_ast.h"

#ifdef HAVE_CONFIG_H
#   include <config.h>
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


/**
 * TODO: Actually handle unescaping. For now we're just removing
 *       double quotes.
 * */
static char *unescape(const char *arg, size_t len)
{
    char *unescaped = calloc(len + 1, sizeof(char));
    size_t uidx = 0;
    for (size_t i = 0; i < len; i++) {
        if (arg[i] != '"') {
            unescaped[uidx] = arg[i];
            ++uidx;
        }
    } 

    return unescaped;
}

static char *__cur_arg = NULL;
static char *get_arg(char *cmd)
{
    /* If __cur_arg is back to the default state,
     * we have received a new command. */
    if (!__cur_arg) {
        __cur_arg = cmd;
    }

    size_t rem_len = strlen(__cur_arg);

    while (isspace(*__cur_arg)) {
        ++__cur_arg;
    }

    /* Count length as long as current char is not null
     * or whitespace. */
    size_t len = 0;
    int in_quotes = 0;
    while (__cur_arg[len] && (in_quotes || !isspace(__cur_arg[len])) && len < rem_len) {
        if (__cur_arg[len] == '"')
            in_quotes = !in_quotes;

        ++len;
    }

    /* Unclosed quotes */
    if (in_quotes) {
        printf("Double quotes unclosed!\n");
    }

    /* If length is 0, we have reached the end of the command. */
    if (!len) {
        __cur_arg = NULL;
        return NULL;
    }

    // char *arg = calloc(len + 1, sizeof(char));
    // strncpy(arg, __cur_arg, len);

    char *arg = unescape(__cur_arg, len);

    __cur_arg += len;

    return arg;
}

Vec_t *parse(char *cmd)
{
    Vec_t *argv = vec_create();
    char *arg = NULL;
    while ((arg = get_arg(cmd))) {
        argv = vec_append(argv, arg);
    }

    return argv;
}

int run_builtin(Vec_t *argv)
{
    if (strcmp(argv->v[0], "cd") == 0) {
        printf("Got builtin command %s\n", argv->v[0]);
    } else if (strcmp(argv->v[0], "alias") == 0) {
        printf("Got builtin command %s\n", argv->v[0]);
    } else {
        return 0;
    }

    return 1;
}

static void run_simple_command(ASTNode_t *cmd)
{
    if (cmd->type != SIMPLE_COMMAND) {
        fprintf(stderr, "run_simple_command() called for cmd->type = %d\n", cmd->type);
        abort();
    }

    pid_t pid = fork();
    pid_t w;
    int wstatus;
    switch (pid) {
    case -1:
        perror("fork");
        break;

    case 0:
        if (execvp(cmd->argv->v[0], cmd->argv->v) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        break;

    default:
        w = waitpid(pid, &wstatus, 0);
    }
}

static void eval(ASTNode_t *root)
{
    if (!root) {
        return;
    }

    if (root->left) {
        eval(root->left);
    }

    if (root->right) {
        eval(root->right);
    }

    switch (root->type) {
    /* A COMMAND_LIST node will always be above a SIMPLE_COMMAND node.
     * Since all of it's children have already been executed, there's
     * nothing to do here. */
    case COMMAND_LIST:
        break;

    default:
        run_simple_command(root);
    }
}

int main(int argc, const char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "--debug") == 0)
        yydebug = 1;

    char *cmd = NULL;
    while ((cmd = readline(PACKAGE_NAME "-" PACKAGE_VERSION "$ "))) {
        cur_cmd = cmd;
        cur_ch = cur_cmd;

        ASTNode_t *root;
        yyparse(&root);

        eval(root);

        // ast_print(root);
        ast_free(root);

        free(cmd);
    }

    return 0;
}

// int main(void)
// {
//     char *cmd = NULL;
//     while ((cmd = readline(PACKAGE_STRING "$ "))) {
//         Vec_t *argv = parse(cmd);
//         if (!argv->sz)
//             continue;
// 
//         if (run_builtin(argv)) {
//             printf("Ran builtin command.\n");
//         } else {
//             pid_t pid = fork();
//             switch (pid) {
//             case -1:
//                 perror("fork");
//                 exit(EXIT_FAILURE);
// 
//             case 0:
//                 if (execvp(argv->v[0], argv->v)) {
//                     perror("execvp");
//                     exit(EXIT_FAILURE);
//                 }
// 
//                 break;
// 
//             /* Parent (shell) process */
//             default:
//                 pid_t w = waitpid(pid, NULL, 0);
//                 if (w == -1) {
//                     perror("waitpid");
//                 }
//             }
//         }
// 
//         free(cmd);
//     }
// 
//     return 0;
// }
