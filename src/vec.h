#ifndef VEC_H
#define VEC_H

#include <stdlib.h>

#define INIT_VEC_SZ            10

typedef struct Vec
{
    char **v;
    size_t sz;
    size_t __pool_sz;
} Vec_t;

Vec_t *vec_create(void);
Vec_t *vec_append(Vec_t *vec, char *s);
void vec_print(Vec_t *vec);
void vec_free(Vec_t *vec);

#endif      /* VEC_H */
