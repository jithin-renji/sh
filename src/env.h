#ifndef ENV_H
#define ENV_H

#include <stddef.h>

#define INIT_ENV_LIST_POOL_SZ           20


typedef struct EnvRec
{
    char *var;
    char *val;
} EnvRec_t;

typedef struct EnvList
{
    EnvRec_t pair;
    size_t sz;
    size_t __pool_sz;
} EnvList_t;

void env_init(const char *envp[]);
char *env_get(const char *name);
void env_set(const char *var, const char *val);
void env_append(const char *var, const char *val);
void env_print(void);
void env_free(void);

#endif  /* ENV_H */
