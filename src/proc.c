#include "proc.h"

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


Pipeline_t *pipeline_create(void)
{
    Pipeline_t *pipeline = malloc(sizeof(Pipeline_t));
    pipeline->argv = NULL;
    pipeline->in_fname = NULL;
    pipeline->out_fname = NULL;
    pipeline->pid = -1;
    pipeline->completed = 0;
    pipeline->next = NULL;

    return pipeline;
}

Proc_t *proc_create(ASTNode_t *cmd)
{
    Proc_t *proc = pipeline_create();
    proc->argv = cmd->argv;
    if (cmd->in_fname) {
        proc->in_fname = strdup(cmd->in_fname);
    }

    if (cmd->out_fname) {
        proc->out_fname = strdup(cmd->out_fname);
        printf("out_fname: '%s'\n", proc->out_fname);
    }

    proc->outfile_append = cmd->outfile_append;
    proc->pid = -1;
    proc->completed = 0;

    return proc;
}

Proc_t *proc_find(Proc_t *pipeline, pid_t pid)
{
    Proc_t *cur = pipeline;
    while (cur) {
        if (cur->pid == pid) {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
}

void pipeline_append(Pipeline_t *pipeline, Proc_t *proc)
{
    Proc_t *cur = pipeline;
    if (!cur->argv) {
        cur->argv = proc->argv;
        cur->in_fname = proc->in_fname;
        cur->out_fname = proc->out_fname;
        cur->outfile_append = proc->outfile_append;
        cur->pid = proc->pid;
        cur->completed = proc->completed;
    } else {
        while (cur) {
            if (!cur->next) {
                cur->next = proc;
                break;
            }

            cur = cur->next;
        }
    }
}

void pipeline_free(Pipeline_t *pipeline)
{
    if (!pipeline)
        return;

    /* argv is freed elsewhere */
    Pipeline_t *cur = pipeline;
    while (cur->next) {
        Pipeline_t *next = cur->next;
        free(cur->in_fname);
        free(cur->out_fname);
        free(cur);

        cur = next;
    }
}

static void reset_signals(void)
{
    signal(SIGINT,  SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

void proc_exec(Proc_t *proc, pid_t pgrp, 
        int read_fd, int write_fd, int is_foreground)
{
    pid_t pid = getpid();

    /* In a pipeline, the first call to this function would have pgrp == 0.
     * In that case, we want the process group to be the same as our PID.*/
    if (pgrp == 0) {
        pgrp = pid;
    }

    setpgid(pid, pgrp);
    if (is_foreground) {
        tcsetpgrp(STDIN_FILENO, pgrp);
    }

    reset_signals();

    if (read_fd != STDIN_FILENO) {
        if (dup2(read_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(read_fd);
    }

    if (write_fd != STDOUT_FILENO) {
        if (dup2(write_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(write_fd);
    }

    execvp(proc->argv->v[0], proc->argv->v);
    if (errno == ENOENT) {
        fprintf(stderr, PACKAGE_NAME ": %s: command not found\n", proc->argv->v[0]);
    } else {
        perror("execvp");
    }

    exit(EXIT_FAILURE);
}
