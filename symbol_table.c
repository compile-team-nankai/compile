#include "symbol_table.h"

symbol_table_t *new_scope(symbol_table_t *father) {
    symbol_table_t *table = (symbol_table_t *)malloc(sizeof(symbol_table_t));
    table->head = NULL;
    table->cur_child = 0;
    table->children_num = 0;
    table->children = NULL;
    table->prev = father;
    if (father) {
        symbol_table_t **tmp = father->children;
        father->children = malloc(sizeof(symbol_table_t *) * (father->children_num + 1));
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
        insert_symbol(table, printf);
        insert_symbol(table, scanf);
    }
    return table;
}

symbol_t *find_symbol(symbol_table_t *table, char *key) {
    symbol_table_item_t *tmp = NULL;
    if (table->head) {
        HASH_FIND_STR(table->head, key, tmp);
    }
    if (tmp == NULL) {
        if (table->prev) {
            return find_symbol(table->prev, key);
        }
        return NULL;
    }
    return &tmp->symbol;
}

void insert_symbol(symbol_table_t *table, symbol_t symbol) {
    symbol_table_item_t *it;
    HASH_FIND_STR(table->head, symbol.name, it);
    if (it == NULL) {
        symbol_table_item_t *tmp = malloc(sizeof(symbol_table_item_t));
        tmp->key = symbol.name;
        tmp->symbol = symbol;
        HASH_ADD_KEYPTR(hh, table->head, symbol.name, strlen(symbol.name), tmp);
    } else {
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
