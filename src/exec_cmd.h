#ifndef EXEC_CMD_H
#define EXEC_CMD_H

#include "vec.h"

typedef struct Proc
{
    Vec_t *argv;
    struct Proc *next;
} Proc_t;

typedef Proc_t Pipeline_t;

/* TODO: Implement job control */
typedef struct Job
{
    size_t id;
    Pipeline_t *pipeline; /* This will probably have to change later */
    int is_foreground;
    int is_running;
    struct Job *next;
} Job_t;

Pipeline_t *pipeline_create(void);
void pipeline_append(Pipeline_t *pipeline, Proc_t *proc);
Proc_t *proc_create(Vec_t *argv);
void pipeline_free(Pipeline_t *pipeline);

void job_create(Pipeline_t *pipeline, int foreground);

#endif
