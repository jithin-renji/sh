#include "cmd_ast.h"
#include "vec.h"

#include <stdio.h>
#include <stdlib.h>

ASTNode_t *ast_node_create(NodeType_t type, Vec_t *argv, ASTNode_t *left, ASTNode_t *right)
{
    ASTNode_t *node = malloc(sizeof(ASTNode_t));
    node->type = type;
    node->argv = argv;
    node->left = left;
    node->right = right;

    return node;
}

static void ast_print_real(ASTNode_t *root, int depth)
{
    if (!root) {
        printf("(empty)\n");
        return;
    }

    for (int i = 0; i < depth; i++) {
        printf("\t");
    }

    switch (root->type) {
    case PIPELINE:
        printf("PIPELINE\n");
        break;

    default:
        printf("%s\n", root->argv->v[0]);
    }

    if (root->left) {
        printf("L ");
        ast_print_real(root->left, depth + 1);
    }

    if (root->right) {
        printf("R ");
        ast_print_real(root->right, depth + 1);
    }
}

void ast_print(ASTNode_t *root)
{
    ast_print_real(root, 0);
}

void ast_free(ASTNode_t *root)
{
    if (!root) {
        return;
    }

    if (root->left) {
        ast_free(root->left);
    }

    if (root->right) {
        ast_free(root->right);
    }

    vec_free(root->argv);

    free(root);
}
