#include "eval.h"

#ifdef HAVE_CONFIG_H
#   include <config.h>
#else
#   error "Missing config.h"
#endif

#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/wait.h>

#define NO_PIPE         -1
#define READ_END        0
#define WRITE_END       1

static void run_simple_command(ASTNode_t *cmd, int read_end, int write_end)
{
    if (cmd->type != SIMPLE_COMMAND) {
        fprintf(stderr, "run_simple_command() called for cmd->type = %d\n", cmd->type);
        abort();
    }

    pid_t pid = fork();
    pid_t w;
    int wstatus;
    switch (pid) {
    case -1:
        perror("fork");
        break;

    case 0:
        if (read_end != NO_PIPE) {
            if (dup2(read_end, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            close(read_end);
        }

        if (write_end != NO_PIPE) {
            if (dup2(write_end, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            close(write_end);
        }

        execvp(cmd->argv->v[0], cmd->argv->v);
        if (errno == ENOENT) {
            fprintf(stderr, PACKAGE_NAME ": %s: command not found\n", cmd->argv->v[0]);
        } else {
            perror("execvp");
        }

        exit(EXIT_FAILURE);

        break;

    default:
        w = waitpid(pid, &wstatus, 0);
    }
}

static void run_pipeline(ASTNode_t *root, ASTNode_t *parent, int next_write)
{
    if (root->type != PIPELINE) {
        fprintf(stderr, "run_pipeline() called for root->type = %d\n", root->type);
        exit(EXIT_FAILURE);
    }

    int pfd[2];
    if (pipe(pfd) == -1) {
        perror("pipe");
    }

    if (root->left->type == SIMPLE_COMMAND && root->right->type == SIMPLE_COMMAND) {
        run_simple_command(root->left, NO_PIPE, pfd[WRITE_END]);
        close(pfd[WRITE_END]);

        run_simple_command(root->right, pfd[READ_END], next_write);
        close(pfd[READ_END]);
    } else if (root->left->type == PIPELINE) {
        run_pipeline(root->left, root, pfd[WRITE_END]);
        close(pfd[WRITE_END]);
        if (parent) {
            run_simple_command(root->right, pfd[READ_END], next_write);
        } else {
            run_simple_command(root->right, pfd[READ_END], NO_PIPE);
        }

        close(pfd[READ_END]);
    }
}

void eval(ASTNode_t *root)
{
    if (!root) {
        return;
    }

    if (root->type == PIPELINE) {
        run_pipeline(root, NULL, NO_PIPE);
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
        run_simple_command(root, NO_PIPE, NO_PIPE);
    }
}
