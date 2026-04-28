#include "vec.h"
#include "cmd_ast.h"
#include "eval.h"
#include "jobs.h"
#include "env.h"
#include "colors.h"

#ifdef HAVE_CONFIG_H
#   include <config.h>
#else
#   error "Missing config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

extern int yydebug;
extern int yyparse(ASTNode_t **root);

char *cur_cmd;
char *cur_ch;

int interactive;

void ignore_interactive_signals(void)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

void clara_init(void)
{
    if (isatty(STDIN_FILENO)) {
        interactive = 1;
    }

    if (interactive) {
        pid_t clara_pgid;
        while (tcgetpgrp(STDIN_FILENO) != (clara_pgid = getpgrp())) {
            kill(-clara_pgid, SIGTTIN);
        }

        ignore_interactive_signals();
        signal(SIGCHLD, reap_completed_bg_procs);

        clara_pgid = getpid();
        if (getpgrp() != clara_pgid) {
            if (setpgid(clara_pgid, clara_pgid) == -1) {
                perror("Unable to create shell process group");
                exit(EXIT_FAILURE);
            }
        }

        if (tcsetpgrp(STDIN_FILENO, clara_pgid) == -1) {
            perror("Unable to set foreground");
            exit(EXIT_FAILURE);
        }
    }
}

void init_wd(void)
{
    char *cwd = getcwd(NULL, 0);
    env_set("PWD", cwd);
    env_set("OLDPWD", cwd);
    free(cwd);
}

int main(int argc, const char *argv[], const char *envp[])
{
    clara_init();
    env_init(envp);
    init_wd();
    printf("DEBUG\n");

    if (argc == 2 && strcmp(argv[1], "--debug") == 0)
        yydebug = 1;

    using_history();

    char *cmd = NULL;

    /* TODO: Remove hardcoded colors and add proper user-configurable prompt support */
    while ((cmd = readline(GREEN PACKAGE_NAME "-" PACKAGE_VERSION "$ " RESET))) {
        if (*cmd) {
            add_history(cmd);
        }

        cur_cmd = cmd;
        cur_ch = cur_cmd;

        ASTNode_t *root = NULL;
        if (yyparse(&root) != 1 && root) {
            eval(root);
        }

        ast_free(root);

        free(cmd);
    }

    jobs_free();
    env_free();

    return 0;
}
