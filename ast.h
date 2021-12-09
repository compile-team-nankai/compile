#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef struct node_t {
    char *node_type;
    char *value;
    int children_num;
    int line;
    struct node_t **children;
} node_t;

node_t *new_node(char *node_type, int n, ...);

node_t *new_value(char *node_type, char *value);

node_t *merge_node(node_t *father, node_t *child);

node_t *l_merge_node(node_t *father, node_t *child);

void free_tree(node_t *root);

#endif // AST_H