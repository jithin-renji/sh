#include "eval.h"
#include "builtins.h"
#include "exec_cmd.h"

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

static Proc_t *eval_simple_command(ASTNode_t *cmd)
{
    if (cmd->type != SIMPLE_COMMAND) {
        fprintf(stderr, "eval_simple_command() called for cmd->type = %d\n", cmd->type);
        exit(EXIT_FAILURE);
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
        Proc_t *proc = proc_create(root->left->argv);
        pipeline_append(pipeline, proc);
    }

    if (root->right->type == SIMPLE_COMMAND) {
        Proc_t *proc = proc_create(root->right->argv);
        pipeline_append(pipeline, proc);
    }
}

void eval(ASTNode_t *root)
{
    if (!root) {
        return;
    }

    if (root->type == PIPELINE) {
        Pipeline_t *pipeline = pipeline_create();
        eval_pipeline(root, pipeline);
        pipeline_exec(pipeline);

        pipeline_free(pipeline);
        return;
    }

    if (root->left) {
        eval(root->left);
    }

    if (root->right) {
        eval(root->right);
    }

    switch (root->type) {
    /* A COMMAND_LIST node will always be above a SIMPLE_COMMAND node.
     * Since all of it's children have already been executed, there's
     * nothing to do here. */
    case COMMAND_LIST:
    case PIPELINE:
        break;

    default:
        Proc_t *proc = eval_simple_command(root);
        pipeline_exec(proc);
        pipeline_free(proc);
    }
}
