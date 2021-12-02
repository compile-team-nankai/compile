#include "ast_symbol.h"
#include "intermediate.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

extern FILE *ast_out;

DAG::~DAG() {
    node_dag* head = nullptr;
    node_dag* next = nullptr;
    for (auto itor = node_map.begin(); itor != node_map.end(); ++itor) {
        head = itor->second;
        while(head != nullptr) {
            next = head->next;
            delete head;
            head = next;
        }
    }
    for (auto itor = leaf_map.begin(); itor != leaf_map.end(); ++itor) {
        delete itor->second;
    }
}

int DAG::get_index(node_dag* new_node) {
    int index = -1;
    auto got = node_map.find(new_node->type);
    if (got == node_map.end()) { // 匹配桶失败，新建桶
        new_node->index = ++index_cur;
        index = new_node->index;
        node_map.emplace(new_node->type, new_node);
        print_dag_node(new_node);
    }
    else {
        node_dag* head = got->second;
        while (head != nullptr) {
            if (is_node_equal(head, new_node)) { // 匹配成功，返回索引
                delete new_node; // 不需要新结点，释放内存
                index = head->index;
                break;
            } 
            if (head->next == nullptr) { // 匹配失败，将新结点加入链表
                head->next = new_node;
                new_node->index = ++index_cur;
                index = new_node->index;
                print_dag_node(new_node);
                break;
            }
            head = head->next;
        }
    }
    return index;
}

int DAG::get_index(leaf_dag* new_leaf) {
    int index = -1;
    auto got = leaf_map.find(new_leaf->value);
    if (got == leaf_map.end()) { // 匹配失败，插入
        new_leaf->index = ++index_cur;
        index = new_leaf->index;
        leaf_map.emplace(new_leaf->value, new_leaf);
        print_dag_leaf(new_leaf);
    }
    else {
        index = got->second->index;
        delete new_leaf; // 不需要新结点，释放内存
    }
    return index;
}

bool DAG::is_node_equal(node_dag* a, node_dag* b) {
    return a->type == b->type
        && a->addr1 == b->addr1
        && a->addr2 == b->addr2;
}

void DAG::print_dag_node(node_dag* node) {
    printf("%3d ", ++line_number);
    if (node->type == "minus") {
        printf("$%d = %s $%d\n", node->index, node->type.c_str(), node->addr1);
    }
    else if (node->type == "=") {
        printf("$%d %s $%d\n", node->addr1, node->type.c_str(), node->addr2);
    }
    else {
        printf("$%d = $%d %s $%d\n", node->index, node->addr1, node->type.c_str(), node->addr2);
    } 
}

void DAG::print_dag_leaf(leaf_dag* leaf) {
    printf("%3d ", ++line_number);
    printf("$%d = %s %s\n", leaf->index, leaf->type.c_str(), leaf->value.c_str());
}

void gen_3address_code(node_t *node, symbol_table_t *table, DAG* dag) {
    if (strncmp(node->node_type, "const", 5) == 0) { //常量
        node->index = dag->get_index(new leaf_dag("const", node->value));
    }
    else if (strcmp(node->node_type, "id") == 0) { //标识符
        node->index = dag->get_index(new leaf_dag("id", node->value));
    }
    else if (strlen(node->node_type) == 1) { //算术运算符，赋值符号
        for (int i = 0; i < node->children_num; ++i) {
            gen_3address_code(node->children[i], table, dag);
        }
        if (node->children_num == 2) { //双目运算符和赋值符号
            node->index = dag->get_index(new node_dag(node->node_type, node->children[0]->index, node->children[1]->index));
        }
        else if (node->children_num == 1 && strcmp(node->node_type, "-") == 0) { //单目负号
            node->index = dag->get_index(new node_dag("minus", node->children[0]->index));
        }
    }
    else {
        for (int i = 0; i < node->children_num; ++i) {
            const char *type = node->children[i]->node_type;
            if (strcmp(type, "code block") == 0) {
                symbol_table_t *next = next_scope(table);
                DAG* next_dag= new DAG(dag->index_cur);
                gen_3address_code(node->children[i], next, next_dag);
                dag->index_cur = next_dag->index_cur;
                delete next_dag;
            }
            else {
                gen_3address_code(node->children[i], table, dag);
            }
        }
    }
}

void gen_code(node_t *root) {
    symbol_table_t *table = generate_symbol_table(root);
    DAG* dag = new DAG();
    gen_3address_code(root, table, dag);
    free_table(table);
    delete dag;
}

node_bool *new_node_bool(const char *node_type, int n, ...) {
    node_bool *node = (node_bool*)malloc(sizeof(node_bool));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = n;
    node->children = (node_t**)malloc(sizeof(node_bool *) * n);
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        node->children[i] = va_arg(ap, node_bool*);
    }
    node->true_list = new std::vector<quadruple*>(); // 暂时缺少对应delete
    node->false_list = new std::vector<quadruple*>(); // 暂时缺少对应delete
    return node;
}

void gen_binary_op(address3* op, address3* arg1, address3* arg2, address3* result) {}
void gen_unary_op(address3* op, address3* arg1, address3* result) {}
void gen_assign(address3* arg1, address3* result) {}
void gen_goto(address3* result) {}
void gen_if_goto(address3* op, address3* arg1, address3* result, bool cond) {}
void gen_if_relop(address3* op, address3* arg1, address3* arg2, address3* result) {}
void translate_bool_expr(node_t *node)  {
    if (strcmp(node->node_type, "||") == 0) {
        // backpatch(B1.falselist, M.instr)
        // B.truelist = merge(B1.truelist, B2.truelist)
        // B.falselist = B2.falselist
    }
    else if (strcmp(node->node_type, "&&") == 0) {
        // backpatch(B1.truelist, M.instr)
        // B.truelist = B2.truelist
        // B.falselist
    }
    else if (strcmp(node->node_type, "!") == 0) {
        //B.truelist = B1.falselist
        //B.falselist = B1.truelist
    }
    else if (strcmp(node->node_type, "rel") == 0) {
        //B.truelist = makelist(nextinstr)
        //B.falselist = makelist(nextinstr+1)
        //gen if E1.addr rel.op E2.addr goto_
        //gen goto_
    }
}
