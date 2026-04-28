#ifndef CMD_AST_H
#define CMD_AST_H

#include "vec.h"

typedef enum NodeType
{
    COMMAND_LIST, SIMPLE_COMMAND, PIPELINE, ASYNC_COMMAND
} NodeType_t;

typedef struct ASTNode
{
    NodeType_t type;
    Vec_t *argv;

    char *in_fname;

    char *out_fname;
    int outfile_append;

    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode_t;

ASTNode_t *ast_node_create(NodeType_t type, Vec_t *argv, char *in_fname, char *out_fname,
        int outfile_append, ASTNode_t *left, ASTNode_t *right);
void ast_print(ASTNode_t *root);
void ast_free(ASTNode_t *root);

#endif  /* CMD_AST_H */
