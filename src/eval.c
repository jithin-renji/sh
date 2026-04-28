#include "eval.h"
#include "builtins.h"
#include "jobs.h"
#include "env.h"

#ifdef HAVE_CONFIG_H
#   include <config.h>
#else
#   error "Missing config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static int eval_builtin(ASTNode_t *cmd)
{
    if (cmd->type != SIMPLE_COMMAND) {
        fprintf(stderr, "eval_builtin() called for cmd->type = %d\n", cmd->type);
        exit(EXIT_FAILURE);
    }

    Builtin_t builtin = get_builtin(cmd->argv->v[0]);
    if (!builtin.action) {
        return -1;
    }

    builtin.action(cmd->argv);

    return 0;
}

static void eval_env_vars(ASTNode_t *cmd)
{
    if (cmd->type != SIMPLE_COMMAND) {
        fprintf(stderr, "eval_env_vars() called for cmd->type = %d\n", cmd->type);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < cmd->argv->sz; i++) {
        if (cmd->argv->v[i][0] == '$') {
            char *old = cmd->argv->v[i];
            char *val = env_get(old + 1);

            /* TODO: Remove arg from argv if the variable
             *       doesn't exist.*/
            if (!val) {
                cmd->argv->v[i] = strdup("");
            } else {
                cmd->argv->v[i] = strdup(val);
            }

            free(old);
        }
    }
}

static Proc_t *eval_simple_command(ASTNode_t *cmd)
{
    if (cmd->type != SIMPLE_COMMAND) {
        fprintf(stderr, "eval_simple_command() called for cmd->type = %d\n", cmd->type);
        exit(EXIT_FAILURE);
    }

    eval_env_vars(cmd);

    if (eval_builtin(cmd) == 0) {
        return NULL;
    }


    return proc_create(cmd->argv);
}

static void eval_pipeline(ASTNode_t *root, Pipeline_t *pipeline)
{
    if (root->type != PIPELINE) {
        fprintf(stderr, "eval_pipeline() called for root->type = %d\n", root->type);
        exit(EXIT_FAILURE);
    }

    if (root->left->type == PIPELINE) {
        eval_pipeline(root->left, pipeline);
    } else if (root->left->type == SIMPLE_COMMAND) {
        eval_env_vars(root->left);
        Proc_t *proc = proc_create(root->left->argv);
        pipeline_append(pipeline, proc);
    }

    if (root->right->type == SIMPLE_COMMAND) {
        eval_env_vars(root->right);
        Proc_t *proc = proc_create(root->right->argv);
        pipeline_append(pipeline, proc);
    }
}

void eval(ASTNode_t *root)
{
    if (!root) {
        return;
    }

    Proc_t * proc;
    Pipeline_t *pipeline;
    switch (root->type) {
    /* A COMMAND_LIST node will always be above a SIMPLE_COMMAND node.
     * Since all of it's children have already been executed, there's
     * nothing to do here. */
    case COMMAND_LIST:
        if (root->left) {
            eval(root->left);
        }

        if (root->right) {
            eval(root->right);
        }

        break;

    case PIPELINE:
        pipeline = pipeline_create();
        eval_pipeline(root, pipeline);
        job_create(pipeline, 1);
        break;

    case ASYNC_COMMAND:
        if (root->left->type == SIMPLE_COMMAND) {
            proc = eval_simple_command(root->left);
            job_create(proc, 0);
        } else if (root->left->type == PIPELINE) {
            pipeline = pipeline_create();
            eval_pipeline(root->left, pipeline);
            job_create(pipeline, 0);
        }

        break;

    default:
        if (root->out_fname) {
            printf("Got file: %s\n", root->out_fname);
        }
        proc = eval_simple_command(root);
        if (proc) {
            job_create(proc, 1);
        }
    }
}
