#include "env.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

EnvList_t *env_list;

static void env_list_grow_if_needed(void)
{
    if (env_list->sz + 1 >= env_list->__pool_sz) {
        env_list->__pool_sz *= 2;
        env_list = realloc(env_list, env_list->__pool_sz * sizeof(EnvList_t));
    }
}

void env_init(const char *envp[])
{
    env_list = malloc(INIT_ENV_LIST_POOL_SZ * sizeof(EnvList_t));
    env_list->__pool_sz = INIT_ENV_LIST_POOL_SZ;
    env_list->sz = 0;
    for (int i = 0; envp[i]; i++) {
        env_list_grow_if_needed();
        for (int j = 0; envp[i][j]; j++) {
            if (envp[i][j] == '=') {
                env_list[i].pair.var = strndup(envp[i], j);
                env_list[i].pair.val = strndup(envp[i] + j + 1, strlen(envp[i]) - (j + 1));
                env_list->sz++;

                break;
            }
        }
    }
}

void env_set(const char *var, const char *val)
{
    int idx = -1;
    for (int i = 0; i < env_list->sz; i++) {
        if (strcmp(env_list[i].pair.var, var) == 0) {
            idx = i;
        }
    }

    if (idx == -1) {
        return;
    }

    free(env_list[idx].pair.val);
    env_list[idx].pair.val = strdup(val);
}

char *env_get(const char *name)
{
    for (int i = 0; i < env_list->sz; i++) {
        if (strcmp(env_list[i].pair.var, name) == 0) {
            return env_list[i].pair.val;
        }
    }

    return NULL;
}

void env_print(void)
{
    for (int i = 0; i < env_list->sz; i++) {
        printf("%d '%s' = '%s'\n", i + 1, env_list[i].pair.var, env_list[i].pair.val);
    }
}

void env_free(void)
{
    for (int i = 0; i < env_list->sz; i++) {
        free(env_list[i].pair.var);
        free(env_list[i].pair.val);
    }

    free(env_list);
}
