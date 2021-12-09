#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>

char error[1024] = {'\0'};

symbol_table_t *new_scope(symbol_table_t *father) {
    symbol_table_t *table = (symbol_table_t *)malloc(sizeof(symbol_table_t));
    table->head = NULL;
    table->cur_child = 0;
    table->children_num = 0;
    table->children = NULL;
    table->prev = father;
    if (father) {
        symbol_table_t **tmp = father->children;
        father->children =
            malloc(sizeof(symbol_table_t *) * (father->children_num + 1));
        for (int i = 0; i < father->children_num; ++i) {
            father->children[i] = tmp[i];
        }
        father->children[father->children_num] = table;
        father->children_num += 1;
        free(tmp);
    } else {
        symbol_t printf = {"printf"};
        symbol_t scanf = {"scanf"};
        printf.type = malloc(sizeof(char) * strlen("(int)function"));
        scanf.type = malloc(sizeof(char) * strlen("(int)function"));
        strcpy(printf.type, "(int)function");
        strcpy(scanf.type, "(int)function");
        insert_symbol(table, printf, 0);
        insert_symbol(table, scanf, 0);
    }
    return table;
}

symbol_t *find_symbol(symbol_table_t *table, char *key1) {
    char key_line[100] = {'\0'};
    strcpy(key_line, key1);
    symbol_table_item_t *tmp = NULL;
    if (table->head) {
        char *key = strtok(key1, "?");
        HASH_FIND_STR(table->head, key, tmp);
    }
    if (tmp == NULL) {
        if (table->prev) {
            return find_symbol(table->prev, key_line);
        }
        char *key = strtok(key_line, "?");
        char *l = strtok(NULL, "?");
        if (l != NULL) {
            char e_error[100];
            sprintf(e_error, "%s%s%s%s", l, ": Error! variable ", key,
                    " not defined\n");
            strcat(error, e_error);
        }
        return NULL;
    }
    return &tmp->symbol;
}

void insert_symbol(symbol_table_t *table, symbol_t symbol, int line) {
    symbol_table_item_t *it;
    HASH_FIND_STR(table->head, symbol.name, it);
    if (it == NULL) {
        symbol_table_item_t *tmp = malloc(sizeof(symbol_table_item_t));
        tmp->key = symbol.name;
        tmp->symbol = symbol;
        HASH_ADD_KEYPTR(hh, table->head, symbol.name, strlen(symbol.name), tmp);
    } else {
        char e_str[100];
        sprintf(e_str, "%s %d %s %s %s", "line", line, ": Error! variable",
                it->symbol.name, "already defined\n");
        strcat(error, e_str);
        it->symbol = symbol;
    }
}

void free_table(symbol_table_t *table) {
    symbol_table_item_t *cur, *tmp;
    HASH_ITER(hh, table->head, cur, tmp) {
        HASH_DELETE(hh, table->head, cur);
        free(cur->symbol.type);
        free(cur);
    }
}

symbol_table_t *next_scope(symbol_table_t *father) {
    if (!father) {
        return NULL;
    }
    if (father->cur_child < father->children_num) {
        return father->children[father->cur_child++];
    }
    return NULL;
}

void print_error() {
    printf("%s", error);
}
