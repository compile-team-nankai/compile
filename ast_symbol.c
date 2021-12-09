#include "ast_symbol.h"

FILE *ast_out;

extern void handle_node(node_t *node, symbol_table_t *table);

extern void print_sub_tree(node_t *node, int depth, symbol_table_t *table);

void handle_declare(node_t *declare_node, symbol_table_t *table) {
    char *type = declare_node->children[0]->node_type;
    symbol_t symbol;
    for (int i = 1; i < declare_node->children_num; ++i) {
        node_t *clause = declare_node->children[i];
        if (strcmp(clause->node_type, "declare clause") != 0) {
            continue;
        }
        if (strcmp(clause->children[0]->node_type, "id") == 0) {
            symbol.type = strdup(type);
            symbol.name = clause->children[0]->value;
        } else if (strcmp(clause->children[0]->node_type, "pointer") == 0) {
            node_t *pointer = clause->children[0];
            symbol.type =
                malloc(sizeof(char) * (strlen(type) + pointer->children_num));
            strcpy(symbol.type, type);
            for (int j = 0; j < pointer->children_num - 1; ++j) {
                strcat(symbol.type, "*");
            }
            symbol.name = pointer->children[pointer->children_num - 1]->value;
        } else if (strcmp(clause->children[0]->node_type, "array") == 0) {
            node_t *array = clause->children[0];
            symbol.type = malloc(sizeof(char) * (strlen(type) + array->children_num * 2 - 1));
            strcpy(symbol.type, type);
            for (int j = 0; j < array->children_num - 1; ++j) {
                strcat(symbol.type, "[]");
            }
            symbol.name = array->children[0]->value;
        }
        insert_symbol(table, symbol, declare_node->line);
    }
}

void handle_function_declare(node_t *function_node, symbol_table_t *table) {
    symbol_t symbol;
    symbol.name = function_node->children[1]->value;
    size_t args_len = 0;
    symbol_table_t *function_scope = new_scope(table);
    for (int i = 0; i < function_node->children[2]->children_num; ++i) {
        node_t *clause = function_node->children[2]->children[i];
        args_len += i != 0;
        args_len += strlen(clause->children[0]->node_type);
        handle_declare(clause, function_scope);
    }
    handle_node(function_node->children[3], function_scope);
    symbol.type =
        malloc(sizeof(char) * (strlen(function_node->children[0]->node_type) + 3 + args_len + 1));
    strcpy(symbol.type, function_node->children[0]->node_type);
    strcat(symbol.type, " (");
    for (int i = 0; i < function_node->children[2]->children_num; ++i) {
        if (i != 0) {
            strcat(symbol.type, ",");
        }
        strcat(symbol.type,
               function_node->children[2]->children[i]->children[0]->node_type);
    }
    strcat(symbol.type, ")");
    insert_symbol(table, symbol, 0);
}

void handle_for_statement(node_t *for_node, symbol_table_t *father) {
    symbol_table_t *table = new_scope(father);
    node_t *first_expr = for_node->children_num < 6 ? for_node->children[0]->children[0] : for_node->children[0];
    if (strcmp(first_expr->node_type, "declare") == 0) {
        handle_declare(first_expr, table);
    }
    if (for_node->children_num < 6) {
        handle_node(for_node->children[1], table);
    } else {
        handle_node(for_node->children[7], table);
    }
}

void handle_node(node_t *node, symbol_table_t *table) {
    node_t *child;
    for (int i = 0; i < node->children_num; ++i) {
        child = node->children[i];
        if (strcmp(child->node_type, "declare function") == 0) {
            handle_function_declare(child, table);
        } else if (strcmp(child->node_type, "for statement") == 0) {
            handle_for_statement(child, table);
        } else if (strcmp(child->node_type, "declare") == 0) {
            handle_declare(child, table);
        } else if (strcmp(child->node_type, "code block") == 0) {
            symbol_table_t *child_scope = new_scope(table);
            handle_node(child, child_scope);
        } else if (strcmp(child->node_type, "declare struct") == 0) {
            continue;
        } else {
            handle_node(child, table);
        }
    }
}

symbol_table_t *generate_symbol_table(node_t *root) {
    symbol_table_t *table = new_scope(NULL);
    handle_node(root, table);
    return table;
}

void print_function_tree(node_t *function_node,
                         int depth,
                         symbol_table_t *table) {
    for (int i = 0; i < depth; ++i) {
        fprintf(ast_out, "|   ");
    }
    fprintf(ast_out, "|-- %s", function_node->node_type);
    fprintf(ast_out, " (%d)\n", function_node->children_num);
    print_sub_tree(function_node->children[0], depth + 1, table);
    print_sub_tree(function_node->children[1], depth + 1, table);
    symbol_table_t *next = next_scope(table);
    print_sub_tree(function_node->children[2], depth + 1, next);
    print_sub_tree(function_node->children[3], depth + 1, next);
}

void print_for_tree(node_t *for_node, int depth, symbol_table_t *table) {
    for (int i = 0; i < depth; ++i) {
        fprintf(ast_out, "|   ");
    }
    fprintf(ast_out, "|-- %s", for_node->node_type);
    fprintf(ast_out, " (%d)\n", for_node->children_num);
    symbol_table_t *next = next_scope(table);
    print_sub_tree(for_node->children[0], depth + 1, next);
    print_sub_tree(for_node->children[1], depth + 1, next);
}

void print_tree_without_symbol(node_t *node, int depth) {
    for (int i = 0; i < depth; ++i) {
        fprintf(ast_out, "|   ");
    }
    fprintf(ast_out, "|-- %s", node->node_type);
    if (strcmp(node->node_type, "id") == 0) {
        fprintf(ast_out, " %s", node->value);
    }
    fprintf(ast_out, " (%d)\n", node->children_num);
    for (int i = 0; i < node->children_num; ++i) {
        print_tree_without_symbol(node->children[i], depth + 1);
    }
}

void print_member_selection(node_t *member_selection_node,
                            int depth,
                            symbol_table_t *table) {
    for (int i = 0; i < depth; ++i) {
        fprintf(ast_out, "|   ");
    }
    fprintf(ast_out, "|-- %s", member_selection_node->node_type);
    fprintf(ast_out, " (%d)\n", member_selection_node->children_num);
    print_sub_tree(member_selection_node->children[0], depth + 1, table);
    print_sub_tree(member_selection_node->children[1], depth + 1, table);
    print_tree_without_symbol(member_selection_node->children[2], depth + 1);
}

void print_sub_tree(node_t *node, int depth, symbol_table_t *table) {
    for (int i = 0; i < depth; ++i) {
        fprintf(ast_out, "|   ");
    }
    fprintf(ast_out, "|-- %s", node->node_type);
    if (strncmp(node->node_type, "const", 5) == 0) {
        fprintf(ast_out, " %s", node->value);
    } else if (strcmp(node->node_type, "id") == 0) {
        symbol_t *symbol = find_symbol(table, node->value);
        fprintf(ast_out, " <%s, %s, %lu>", symbol->type, symbol->name,
                (unsigned long)symbol);
    }
    fprintf(ast_out, " (%d)\n", node->children_num);
    for (int i = 0; i < node->children_num; ++i) {
        char *type = node->children[i]->node_type;
        if (strcmp(type, "declare function") == 0) {
            print_function_tree(node->children[i], depth + 1, table);
        } else if (strcmp(type, "for statement") == 0) {
            print_for_tree(node->children[i], depth + 1, table);
        } else if (strcmp(type, "code block") == 0) {
            symbol_table_t *next = next_scope(table);
            print_sub_tree(node->children[i], depth + 1, next);
        } else if (strcmp(type, "declare struct") == 0) {
            print_tree_without_symbol(node->children[i], depth + 1);
        } else if (strcmp(type, "member selection") == 0) {
            print_member_selection(node->children[i], depth + 1, table);
        } else {
            print_sub_tree(node->children[i], depth + 1, table);
        }
    }
}

void print_tree(node_t *root) {
    symbol_table_t *table = generate_symbol_table(root);
    print_sub_tree(root, 0, table);
    free_table(table);
}
