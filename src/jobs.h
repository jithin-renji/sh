#ifndef JOBS_H
#define JOBS_H

#include "proc.h"

/* TODO: Implement job control */
typedef struct Job
{
    size_t id;
    char *cmdline;          /* Future */
    Pipeline_t *pipeline;   /* This will probably have to change later */
    pid_t pgrp;
    int is_foreground;
    int is_running;
    struct Job *next;
} Job_t;

extern Job_t *jobs;

void job_create(Pipeline_t *pipeline, int foreground);

void job_wait(Job_t *job);
Job_t *job_find(size_t id);
Job_t *job_find_by_pid(pid_t pid);
void reap_completed_bg_procs(int s);

int job_is_completed(Job_t *job);

int job_fg(Job_t *job);
int job_bg(Job_t *job);
void job_free(Job_t *job);
void jobs_free(void);

#endif  /* JOBS_H */
