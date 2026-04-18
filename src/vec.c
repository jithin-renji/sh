#include "vec.h"

#include <stdio.h>

Vec_t *vec_create(void)
{
    Vec_t *vec = malloc(sizeof(Vec_t));
    vec->v = malloc(INIT_VEC_SZ * sizeof(char *));
    vec->v[0] = NULL;
    vec->sz = 0;
    vec->__pool_sz = INIT_VEC_SZ;

    return vec;
}

Vec_t *vec_append(Vec_t *vec, char *s)
{
    if (vec->sz + 1 < vec->__pool_sz) {
        vec->__pool_sz *= 2;
        vec->v = realloc(vec->v, vec->__pool_sz);
    }

    vec->v[vec->sz] = s;
    vec->sz++;
    vec->v[vec->sz] = NULL;

    return vec;
}

void vec_print(Vec_t *vec)
{
    printf("VEC:\n");
    for (size_t i = 0; i < vec->sz + 1; i++) {
        if (vec->v[i]) {
            printf("\t%s\n", vec->v[i]);
        } else {
            printf("\t(NULL)\n");
        }
    }
}

void vec_free(Vec_t *vec)
{
    for (size_t i = 0; i < vec->sz; i++) {
        free(vec->v[i]);
    }

    free(vec->v);
    free(vec);
}
