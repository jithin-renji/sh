#include "jobs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/wait.h>

Job_t *jobs = NULL;
static size_t next_job_id = 0;

static void job_add(Job_t *job)
{
    if (!jobs) {
        jobs = job;
    } else {
        /* Add job to the beginning of list */
        job->next = jobs;
        jobs = job;
    }
}

static void job_remove(Job_t *job)
{
    Job_t *cur = jobs;
    while (cur) {
        if (cur->next == job) {
            cur->next = job->next;
            job_free(job);
            break;
        } else if (cur == job) {
            job_free(job);
            jobs = NULL;
            break;
        }

        cur = cur->next;
    }
}

void job_wait(Job_t *job)
{
    int wstatus;
    pid_t pid = waitpid(-job->pgrp, &wstatus, WUNTRACED);
    if (pid == -1) {
        if (errno != ECHILD) {
            perror("waitpid");
        }

        job_remove(job);
    } else {
        if (WIFSTOPPED(wstatus)) {
            printf("\n[%ld] Stopped\n"
                   "Run 'bg' to send this job to the background.\n"
                   "Run 'fg' to bring this job back to the foreground.\n", job->id);
        } else if (WIFEXITED(wstatus)) {
            job_remove(job);
        }
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
}

int job_fg(Job_t *job)
{
    if (kill(-job->pgrp, SIGCONT) == -1) {
        perror("Unable to resume job");
        return -1;
    }

    fprintf(stderr, "%s\n", jobs->cmdline);
    tcsetpgrp(STDIN_FILENO, job->pgrp);
    job_wait(job);

    return 0;
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

    Job_t *job = malloc(sizeof(Job_t));
    job->id = next_job_id;
    job->cmdline = strdup(pipeline->argv->v[0]);
    job->pipeline = pipeline;
    job->pgrp = pgrp;
    job->is_foreground = 0;
    job->is_running = 0;
    job->next = NULL;

    job_add(job);
    next_job_id++;

    job_wait(job);
}

void job_free(Job_t *job)
{
    free(job->cmdline);
    pipeline_free(job->pipeline);
    free(job);
}

void jobs_free(void)
{
    Job_t *cur = jobs;
    while (cur) {
        Job_t *next = cur->next;
        job_free(cur);
        cur = next;
    }
}
