#ifndef COMPILE_AST_SYMBOL_H
#define COMPILE_AST_SYMBOL_H

#include "ast.h"
#include "symbol_table.h"

//symbol_table_t *generate_symbol_table(node_t *root);

void handle_node(node_t *node, symbol_table_t *table);
void handle_function_declare(node_t *function_node, symbol_table_t *table);
void handle_for_statement(node_t *for_node, symbol_table_t *father);
void handle_node(node_t *node, symbol_table_t *table);
symbol_table_t *generate_symbol_table(node_t *root);
void print_sub_tree(node_t *node, int depth, symbol_table_t *table);
void print_function_tree(node_t *function_node, int depth, symbol_table_t *table);
void print_for_tree(node_t *for_node, int depth, symbol_table_t *table);
void print_tree_without_symbol(node_t *node, int depth);
void print_member_selection(node_t *member_selection_node, int depth, symbol_table_t *table);
void print_sub_tree(node_t *node, int depth, symbol_table_t *table);
void print_tree(node_t *root);

#endif //COMPILE_AST_SYMBOL_H
