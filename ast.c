#include "ast.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

node_t* new_node(char* node_type, int n, ...) {
    int m = n;
    if (strcmp(node_type, "declare") == 0 && n == 3) {
        m = n - 1;
    }
    node_t* node = malloc(sizeof(node_t));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = m;
    node->children = malloc(sizeof(node_t*) * m);
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        if (strcmp(node_type, "declare") == 0 && i == 2) {
            node->line = va_arg(ap, int);
        }
        node->children[i] = va_arg(ap, node_t*);
    }
    return node;
}

node_t* new_value(char* node_type, char* value) {
    node_t* node = malloc(sizeof(node_t));
    node->node_type = node_type;
    node->value = value;
    node->children_num = 0;
    node->children = NULL;
    return node;
}

node_t* merge_node(node_t* father, node_t* child) {
    father->children_num += 1;
    node_t** temp = father->children;
    father->children = malloc(sizeof(node_t*) * father->children_num);
    for (int i = 0; i < father->children_num - 1; ++i) {
        father->children[i] = temp[i];
    }
    father->children[father->children_num - 1] = child;
    free(temp);
    return father;
}

node_t* l_merge_node(node_t* father, node_t* child) {
    father->children_num += 1;
    node_t** temp = father->children;
    father->children = malloc(sizeof(node_t*) * father->children_num);
    for (int i = 0; i < father->children_num - 1; ++i) {
        father->children[i + 1] = temp[i];
    }
    father->children[0] = child;
    free(temp);
    return father;
}

void free_tree(node_t* root) {
    for (int i = 0; i < root->children_num; ++i) {
        free_tree(root->children[i]);
    }
    free(root->value);
    free(root);
}
