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
    Job_t *prev = NULL;
    while (cur) {
        if (cur == job) {
            if (prev) {
                prev->next = job->next;
            } else {
                jobs = job->next;
            }

            job_free(job);
            break;
        }

        prev = cur;
        cur = cur->next;
    }
}

void job_wait(Job_t *job)
{
    int wstatus;
    pid_t pid;
    while ((pid = waitpid(-job->pgrp, &wstatus, WUNTRACED)) > 0) {
        if (WIFSTOPPED(wstatus)) {
            printf("\n[%ld] Stopped\n"
                   "Run 'bg' to send this job to the background.\n"
                   "Run 'fg' to bring this job back to the foreground.\n", job->id);
            job->is_foreground = 0;
            job->is_running = 0;
            break;
        } else if (WIFSIGNALED(wstatus)) {
            printf("\n[%ld] Terminated (signal %d)\n", job->id, WTERMSIG(wstatus));

            /* Remove job regardless of if we marked it as completed
             * because it was terminated by a signal */
            job_remove(job);
            break;
        } else if (WIFEXITED(wstatus)) {
            Proc_t *proc = proc_find(job->pipeline, pid);
            proc->completed = 1;
            if (job_is_completed(job)) {
                job_remove(job);
            }
        }
    }

    if (pid == -1) {
        if (errno != ECHILD) {
            perror("waitpid");
        }
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
}

Job_t *job_find(size_t jid)
{
    for (Job_t *cur = jobs; cur; cur = cur->next) {
        if (cur->id == jid) {
            return cur;
        }
    }

    return NULL;
}

Job_t *job_find_by_pid(pid_t pid)
{
    Job_t *cur = jobs;
    while (cur) {
        Proc_t *cur_proc = cur->pipeline;
        while (cur_proc) {
            if (cur_proc->pid == pid) {
                return cur;
            }

            cur_proc = cur_proc->next;
        }

        cur = cur->next;
    }

    return NULL;
}

void reap_completed_bg_procs(int s)
{
    pid_t pid;
    int wstatus;

    /* Here, we're only reaping a single process at a time. No concept
     * of jobs. Should find a way to remove job when all procs in the
     * job have been reaped. */
    Job_t *job = NULL;
    while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
        job = job_find_by_pid(pid);
        Proc_t *proc = proc_find(job->pipeline, pid);
        if (proc) {
            proc->completed = true;
        }

        if (WIFEXITED(wstatus)) {
            if (job && !job->is_foreground) {
                fprintf(stderr, "\n[%ld] Exited\n", job->id);
            }

            break;
        } else if (WIFSIGNALED(wstatus)) {
            int s = WTERMSIG(wstatus);
            if (job) {
                fprintf(stderr, "\n[%ld] Terminated (signal %d)\n", job->id, s);
            } else {
                /* If we're here, something's wrong */
                fprintf(stderr, "\n[(pid) %d] Terminated (signal %d)\n", pid, s);
            }

            break;
        }
    }

    if (pid != -1 && job && job_is_completed(job)) {
        job_remove(job);
    }
}

int job_is_completed(Job_t *job)
{
    for (Proc_t *proc = job->pipeline; proc; proc = proc->next) {
        if (!proc->completed) {
            return 0;
        }
    }

    return 1;
}

int job_fg(Job_t *job)
{
    if (kill(-job->pgrp, SIGCONT) == -1) {
        perror("Unable to resume job");
        return -1;
    }

    fprintf(stderr, "%s\n", job->cmdline);
    job->is_foreground = 1;
    job->is_running = 1;

    tcsetpgrp(STDIN_FILENO, job->pgrp);
    job_wait(job);

    return 0;
}

int job_bg(Job_t *job)
{
    if (kill(-job->pgrp, SIGCONT) == -1) {
        perror("Unable to resume job");
        return -1;
    }

    fprintf(stderr, "\n[%ld] %s &\n", job->id, job->cmdline);
    job->is_foreground = 0;
    job->is_running = 1;

    tcsetpgrp(STDIN_FILENO, getpgrp());

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
            proc->pid = pid;
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
    job->is_foreground = 1;
    job->is_running = 1;
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
