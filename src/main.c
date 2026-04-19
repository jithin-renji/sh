#include "vec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

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

int main(void)
{
    char prompt[3] = "$ ";
    if (geteuid() == 0) {
        prompt[0] = '#';
    }

    char *cmd = NULL;
    while ((cmd = readline(prompt))) {
        Vec_t *argv = parse(cmd);
        if (run_builtin(argv)) {
            printf("Ran builtin command.\n");
        } else {
            pid_t pid = fork();
            switch (pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);

            case 0:
                if (execvp(argv->v[0], argv->v)) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }

                break;

            /* Parent (shell) process */
            default:
                pid_t w = waitpid(pid, NULL, 0);
                if (w == -1) {
                    perror("waitpid");
                }
            }
        }

        free(cmd);
    }

    return 0;
}
