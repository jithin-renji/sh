#include "builtins.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static int change_working_dir(Vec_t *args);
static int exit_shell(Vec_t *args);

Builtin_t builtins[NUM_BUILTINS + 1] = {
    { "cd", change_working_dir },
    { "exit", exit_shell },
    { "", NULL }
};

Builtin_t get_builtin(char *name)
{
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(name, builtins[i].name) == 0) {
            return builtins[i];
        }
    }

    return builtins[NUM_BUILTINS];
}

static int change_working_dir(Vec_t *args)
{
    if (args->sz == 1) {
        if (chdir(getenv("HOME")) == -1) {
            perror("cd");
            return -1;
        }
    } else if (args->sz == 2) {
        if (chdir(args->v[1]) == -1) {
            if (errno == ENOENT) {
                fprintf(stderr, "cd: %s: No such file or directory\n", args->v[1]);
            } else {
                perror("cd");
            }

            return -1;
        }
    } else {
        fprintf(stderr, "cd: too many arguments\n");
        return -1;
    }

    return 0;
}

static int exit_shell(Vec_t *args)
{
    /* TODO: Return status based on argument */
    exit(EXIT_SUCCESS);
}
