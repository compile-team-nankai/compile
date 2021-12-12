#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include "uthash.h"

typedef struct symbol_t {
    char *name;
    char *type;
    long long offset;
} symbol_t;

typedef struct {
    char *key;
    symbol_t symbol;
    UT_hash_handle hh;
} symbol_table_item_t;

typedef struct symbol_table_t {
    symbol_table_item_t *head;
    int cur_child;
    int children_num;
    struct symbol_table_t *prev;
    struct symbol_table_t **children;
} symbol_table_t;

symbol_table_t *new_scope(symbol_table_t *father);

symbol_table_t *next_scope(symbol_table_t *father);

symbol_t *find_symbol(symbol_table_t *table, char *key);

void insert_symbol(symbol_table_t *table, symbol_t symbol, int line);

void free_table(symbol_table_t *table);

void print_error();

#endif // SYMBOL_TABLE
