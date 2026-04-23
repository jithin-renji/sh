#include "builtins.h"
#include "jobs.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static int change_working_dir(Vec_t *argv);
static int show_job_list(Vec_t *argv);
static int bring_job_foreground(Vec_t *argv);
static int send_job_background(Vec_t *argv);
static int exit_shell(Vec_t *argv);

Builtin_t builtins[NUM_BUILTINS + 1] = {
    { "cd", change_working_dir },
    { "jobs", show_job_list },
    { "fg", bring_job_foreground },
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

static int change_working_dir(Vec_t *argv)
{
    if (argv->sz == 1) {
        if (chdir(getenv("HOME")) == -1) {
            perror("cd");
            return -1;
        }
    } else if (argv->sz == 2) {
        if (chdir(argv->v[1]) == -1) {
            if (errno == ENOENT) {
                fprintf(stderr, "cd: %s: No such file or directory\n", argv->v[1]);
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

static int show_job_list(Vec_t *argv)
{
    Job_t *cur = jobs;
    while (cur) {
        printf("[%ld] %s\n", cur->id, cur->cmdline);
        cur = cur->next;
    }

    return 0;
}

static int bring_job_foreground(Vec_t *argv)
{
    Job_t *job = jobs;
    if (!job) {
        fprintf(stderr, "No stopped jobs to bring to the foreground.\n");
        return -1;
    }

    return job_fg(job);
}

static int send_job_background(Vec_t *argv)
{
    return 0;
}

static int exit_shell(Vec_t *argv)
{
    /* TODO: Return status based on argument */
    exit(EXIT_SUCCESS);
}
