#ifndef BUILTINS_H
#define BUILTINS_H

#include "vec.h"

#define NUM_BUILTINS        4

typedef struct Builtin
{
    char *name;
    int (*action)(Vec_t *);
} Builtin_t;

// extern Builtin_t *builtins;

Builtin_t get_builtin(char *name);

#endif  /* BUILTINS */
