#include "vec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

static char *__cur_arg = NULL;
static char *get_arg(char *cmd)
{
    /* If __cur_arg is back to the default state,
     * we have received a new command. */
    if (!__cur_arg) {
        __cur_arg = cmd;
    }

    while (isspace(*__cur_arg)) {
        ++__cur_arg;
    }

    /* Count length as long as current char is not null
     * or whitespace. */
    size_t len = 0;
    while (__cur_arg[len] && !isspace(__cur_arg[len])) {
        ++len;
    }

    /* If length is 0, we have reached the end of the command. */
    if (!len) {
        __cur_arg = NULL;
        return NULL;
    }

    char *arg = calloc(len + 1, sizeof(char));
    strncpy(arg, __cur_arg, len);

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

int main(void)
{
    char prompt[3] = "$ ";
    if (geteuid() == 0) {
        prompt[0] = '#';
    }

    char *cmd = NULL;
    while ((cmd = readline(prompt))) {
        Vec_t *argv = parse(cmd);
        vec_print(argv);
        free(cmd);
    }

    return 0;
}
