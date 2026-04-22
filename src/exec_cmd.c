#include "exec_cmd.h"

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

Pipeline_t *pipeline_create(void)
{
    Pipeline_t *pipeline = malloc(sizeof(Pipeline_t));
    pipeline->argv = NULL;
    pipeline->next = NULL;

    return pipeline;
}

Proc_t *proc_create(Vec_t *argv)
{
    Proc_t *proc = pipeline_create();
    proc->argv = argv;

    return proc;
}

void pipeline_append(Pipeline_t *pipeline, Proc_t *proc)
{
    Proc_t *cur = pipeline;
    if (!cur->argv) {
        cur->argv = proc->argv;
    } else {
        while (cur->next != NULL) {
            cur = cur->next;
        }

        cur->next = proc;
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
    signal(SIGCHLD, SIG_DFL);
}

void proc_exec(Proc_t *proc, pid_t pgrp, int read_fd, int write_fd)
{
    pid_t pid = getpid();

    /* In a pipeline, the first call to this function would have pgrp == 0.
     * In that case, we want the process group to be the same as our PID.*/
    if (pgrp == 0) {
        pgrp = pid;
    }

    setpgid(pid, pgrp);
    tcsetpgrp(STDIN_FILENO, pgrp);

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

void job_create(Pipeline_t *pipeline, int is_foreground)
{
    int read_fd = STDIN_FILENO;
    int write_fd = STDOUT_FILENO;
    pid_t pgrp = 0;
    for (Proc_t *proc = pipeline; proc; proc = proc->next) {
        int pfd[2];
        if (proc->next) {
            if (pipe(pfd) == -1) {
                perror("pipe");
                break;
            }

            write_fd = pfd[1];
        } else {
            /* Reset write_fd since it may have been overwritten by
             * previous processes. */
            write_fd = STDOUT_FILENO;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            break;
        }

        switch (pid) {
        case 0:
            proc_exec(proc, pgrp, read_fd, write_fd);
            break;

        default:
            /* The first process should be the session leader. */
            if (pgrp == 0) {
                pgrp = pid;
            }

            setpgid(pid, pgrp);
        }

        if (read_fd != STDIN_FILENO) {
            close(read_fd);
        }

        if (write_fd != STDOUT_FILENO) {
            close(write_fd);
        }

        /* The next process in the pipeline should read from *this* pipe. */
        read_fd = pfd[0];
    }

    int wstatus;
    if (waitpid(-pgrp, &wstatus, WUNTRACED) == -1 && errno != ECHILD) {
        perror("waitpid");
    }

    if (WIFSTOPPED(wstatus)) {
        fprintf(stderr, "Stopped.\nRun 'bg' to send this job to the background or 'fg' to bring it back to the foreground.\n");
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
}
