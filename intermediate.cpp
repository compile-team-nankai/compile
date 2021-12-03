#include "ast_symbol.h"
#include "intermediate.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

extern FILE *ast_out;
int nextinstr = 0;
std::vector<quadruple*> quadruple_array;
std::vector<address3*> address3_pool;
long long temp_count = 0;

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

bool DAG::try_get_expr(node_expr* e, node_dag* new_node) {
    int index = -1;
    bool found = false;
    auto got = node_map.find(new_node->type);
    if (got == node_map.end()) { // 匹配桶失败，新建桶
        new_node->addr = e->addr = new_temp(); // 新建临时变量
        new_node->index = index = ++index_cur;
        node_map.emplace(new_node->type, new_node);
    }
    else {
        node_dag* head = got->second;
        while (head != nullptr) {
            if (is_node_equal(head, new_node)) { // 匹配成功
                e->addr = head->addr; // 复制地址
                index = head->index; // 复制标号
                found = true;
                delete new_node; // 不需要新结点，释放内存
                break;
            } 
            else if (head->next == nullptr) { // 匹配失败，将新结点加入链表
                new_node->addr = e->addr = new_temp();
                new_node->index = index = ++index_cur;
                head->next = new_node;
                break;
            }
            head = head->next;
        }
    }
    e->index = index;
    return found;
}

bool DAG::try_get_expr(node_expr* e, std::string type, long long value) {
    int index = -1;
    bool found = false;
    std::string key = type + std::to_string(value);
    auto got = leaf_map.find(key);
    if (got == leaf_map.end()) { // 匹配失败，插入新结点
        address3* addr = nullptr;
        if (type == "expr_const") {
            addr = e->addr = new_temp();
            gen_assign(new_address3_number(value), e->addr);
        }
        else {
            addr = e->addr = new_address3_address(value); 
        }
        leaf_dag* new_leaf = new leaf_dag(addr);
        new_leaf->index = index = ++index_cur;
        leaf_map.emplace(key, new_leaf);
    }
    else { //匹配成功
        e->addr = got->second->addr; // 复制地址
        index = got->second->index; // 复制标号
        found = true;
    }
    e->index = index;
    return found;
}

bool DAG::is_node_equal(node_dag* a, node_dag* b) {
    return a->type == b->type
        && a->index1 == b->index1
        && a->index2 == b->index2;
}

void DAG::print_dag_node(node_dag* node) {
    printf("%4d ", ++line_number);
    if (node->type == "minus") {
        printf("$%d = %s $%d\n", node->index, node->type.c_str(), node->index1);
    }
    else if (node->type == "=") {
        printf("$%d %s $%d\n", node->index1, node->type.c_str(), node->index2);
    }
    else {
        printf("$%d = $%d %s $%d\n", node->index, node->index1, node->type.c_str(), node->index2);
    } 
}

void DAG::print_dag_leaf(leaf_dag* leaf) {
    printf("%4d ", ++line_number);
    print_address3(leaf->addr);
}

void tranverse_tree(node_t *node, symbol_table_t *table, DAG* dag) {
    const char* type = node->node_type;
    node_expr* e = (node_expr*)node;
    node_expr* e1 = node->children_num > 0 ? (node_expr*) node->children[0] : nullptr;
    node_expr* e2 = node->children_num > 1 ? (node_expr*) node->children[1] : nullptr;
    //遍历子结点
    for (int i = 0; i < node->children_num; ++i) {
        if (strcmp(node->children[i]->node_type, "code block") == 0) {
            symbol_table_t *next = next_scope(table);
            tranverse_tree(node->children[i], next, dag);
        }
        else {
            tranverse_tree(node->children[i], table, dag);
        }
    }
    //执行语义规则
    if (strcmp(type, "expr_const") == 0) { //常量
        dag->try_get_expr(e, "expr_const", atoll(e1->value));
    }
    else if (strcmp(type, "expr_id") == 0) { //标识符
        symbol_t *symbol = find_symbol(table, node->children[0]->value);
        dag->try_get_expr(e, "expr_id", (long long) symbol);
    }
    else if (strcmp(type, "=") == 0) { //赋值符号
        e->addr = e1->addr;
        gen_assign(e2->addr, e1->addr);
    }
    else if (strcmp(type, "+") == 0 ||
        strcmp(type, "-") == 0 ||
        strcmp(type, "*") == 0 ||
        strcmp(type, "/") == 0 ||
        strcmp(type, "%") == 0)
    {                               //算术运算符
        if (!dag->try_get_expr(e, new node_dag(type, e1->index, e2->index))) {
            gen_binary_op(type, e1->addr, e2->addr, e->addr);
        }
    }
    else if (strcmp(type, "minus") == 0) { //单目负号
        if (!dag->try_get_expr(e, new node_dag("minus", e1->index))) {
            gen_unary_op(type, e1->addr, e->addr);
        }
    }
}

void gen_code(node_t *root) {
    symbol_table_t *table = generate_symbol_table(root);
    DAG* dag = new DAG();
    tranverse_tree(root, table, dag);
    free_table(table);
    delete dag;
}

void print_quadruples() {
    for (size_t i = 0; i < quadruple_array.size(); ++i) {
        printf("%s ", quadruple_array[i]->op.c_str());
        print_address3(quadruple_array[i]->arg1);
        print_address3(quadruple_array[i]->arg2);
        print_address3(quadruple_array[i]->result);
        printf("\n");
    }
}

void print_address3(address3 *address) {
    if (address == nullptr) { return; }
    if (address->type == "num") { printf("#"); }
    else { printf("$"); }
    printf("%lld ", address->value);
}

node_bool *new_node_bool(const char *node_type, int n, ...) {
    node_bool *node = (node_bool*)malloc(sizeof(node_bool));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = n;
    node->children = (node_t**)malloc(sizeof(node_t *) * n);
    node->true_list = nullptr;
    node->false_list = nullptr;
    node->instr = -1;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        node->children[i] = va_arg(ap, node_t*);
    }
    return node;
}

node_expr *new_node_expr(const char *node_type, int n, ...) {
    node_expr *node = (node_expr*)malloc(sizeof(node_expr));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = n;
    node->children = (node_t**)malloc(sizeof(node_t *) * n);
    node->addr = nullptr;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        node->children[i] = va_arg(ap, node_t*);
    }
    return node;
}

void gen_binary_op(std::string op, address3* arg1, address3* arg2, address3* result) {
    quadruple_array.push_back(new quadruple(op, arg1, arg2, result));
}
void gen_unary_op(std::string op, address3* arg1, address3* result) {
    quadruple_array.push_back(new quadruple(op, arg1, nullptr, result));
}
void gen_assign(address3* arg1, address3* result) {
    quadruple_array.push_back(new quadruple("assign", arg1, nullptr, result));
}
void gen_goto(address3* result) {
    quadruple_array.push_back(new quadruple("goto", nullptr, nullptr, result));
}
void gen_if_goto(address3* arg1, address3* result, bool cond) {
    quadruple_array.push_back(new quadruple("if_goto", arg1, nullptr, result));
}
void gen_if_relop(address3* arg1, address3* arg2, address3* result) {
    quadruple_array.push_back(new quadruple("if_relop", arg1, arg2, result));
}
/*void translate_bool_expr(node_bool *node)  {
    node_bool* b1 = (node_bool*)node->children[0];
    node_bool* b2 = (strcmp(node->node_type, "&&")==0 || strcmp(node->node_type, "||")==0) ? (node_bool*)node->children[1] : nullptr;
    if (strcmp(node->node_type, "||") == 0) {
        // backpatch(B1.falselist, M.instr)
        backpatch(b1->false_list, node->instr);
        // B.truelist = merge(B1.truelist, B2.truelist)
        node->true_list = merge(b1->true_list, b2->false_list);
        // B.falselist = B2.falselist
        node->false_list->assign(b2->false_list->begin(), b2->false_list->end());
    }
    else if (strcmp(node->node_type, "&&") == 0) {
        // backpatch(B1.truelist, M.instr)
        backpatch(b1->false_list, node->instr);
        // B.truelist = B2.truelist
        node->true_list->assign(b2->true_list->begin(), b2->true_list->end());
        // B.falselist = merge(B1.falselist, B2.falselist)
        node->false_list = merge(b1->false_list, b2->false_list);
    }
    else if (strcmp(node->node_type, "!") == 0) {
        //B.truelist = B1.falselist
        node->true_list = b1->false_list;
        //B.falselist = B1.truelist
        node->false_list = b1->true_list;
    }
    else if (strcmp(node->node_type, "rel") == 0) {
        //B.truelist = makelist(nextinstr)
        node->true_list = makelist(nextinstr);
        //B.falselist = makelist(nextinstr+1)
        node->false_list = makelist(nextinstr + 1);
        //gen if E1.addr rel.op E2.addr goto_
        //gen goto_
    }
}*/

address3 *new_address3(std::string type, long long value) {
    address3 *p =  new address3(type, value);
    address3_pool.push_back(p);
    return p;
}
address3 *new_address3(std::string type, const char* value) {
    address3 *p =  new address3(type, atoll(value));
    address3_pool.push_back(p);
    return p;
}
address3 *new_address3_number(long long number) {
    return new_address3("num", number);
}
address3 *new_address3_number(const char* number) {
    return new_address3("num", number);
}
address3 *new_address3_address(long long address) {
    return new_address3("address", address);
}
address3 *new_address3_address(const char* address) {
    return new_address3("address", address);
}
address3 *new_temp() {
    ++temp_count;
    return new_address3_address(temp_count);
}
void free_address3_pool() {
    for (auto itor = address3_pool.begin(); itor != address3_pool.end(); ++itor) {
        delete *itor;
    }
}
void free_quadruples_array() {
    for (auto itor = quadruple_array.begin(); itor != quadruple_array.end(); ++itor) {
        delete *itor;
    }
}
