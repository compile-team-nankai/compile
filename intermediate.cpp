#include "intermediate.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

extern "C" {
#include "ast_symbol.h"
}

extern FILE *ast_out;
std::vector<quadruple *> quadruple_array;
std::vector<address3 *> address3_pool;
std::vector<std::vector<int> *> bool_list_pool;
long long temp_count = 0;

DAG::~DAG() {
    node_dag *head = nullptr;
    node_dag *next = nullptr;
    for (auto itor = node_map.begin(); itor != node_map.end(); ++itor) {
        head = itor->second;
        while (head != nullptr) {
            next = head->next;
            delete head;
            head = next;
        }
    }
    for (auto itor = leaf_map.begin(); itor != leaf_map.end(); ++itor) {
        delete itor->second;
    }
}

bool DAG::try_get_expr(node_expr *e, node_dag *new_node) {
    int index = -1;
    bool found = false;
    auto got = node_map.find(new_node->type);
    if (got == node_map.end()) { // 匹配桶失败，新建桶
        new_node->addr = e->addr = new_temp(); // 新建临时变量
        new_node->index = index = ++index_cur;
        node_map.emplace(new_node->type, new_node);
    } else {
        node_dag *head = got->second;
        while (head != nullptr) {
            if (is_node_equal(head, new_node)) { // 匹配成功
                e->addr = head->addr; // 复制地址
                index = head->index; // 复制标号
                found = true;
                delete new_node; // 不需要新结点，释放内存
                break;
            } else if (head->next == nullptr) { // 匹配失败，将新结点加入链表
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

bool DAG::try_get_expr(node_expr *e, std::string type, long long value) {
    int index = -1;
    bool found = false;
    std::string key = type + std::to_string(value);
    auto got = leaf_map.find(key);
    if (got == leaf_map.end()) { // 匹配失败，插入新结点
        address3 *addr = nullptr;
        if (type == "expr_const") {
            addr = e->addr = new_temp();
            gen_assign(new_address3_number(value), e->addr);
        } else {
            addr = e->addr = new_temp();
        }
        leaf_dag *new_leaf = new leaf_dag(addr);
        new_leaf->index = index = ++index_cur;
        leaf_map.emplace(key, new_leaf);
    } else { //匹配成功
        e->addr = got->second->addr; // 复制地址
        index = got->second->index; // 复制标号
        found = true;
    }
    e->index = index;
    return found;
}

bool DAG::is_node_equal(node_dag *a, node_dag *b) {
    return a->type == b->type
           && a->index1 == b->index1
           && a->index2 == b->index2;
}

void DAG::print_dag_node(node_dag *node) {
    printf("%4d ", ++line_number);
    if (node->type == "minus") {
        printf("$%d = %s $%d\n", node->index, node->type.c_str(), node->index1);
    } else if (node->type == "=") {
        printf("$%d %s $%d\n", node->index1, node->type.c_str(), node->index2);
    } else {
        printf("$%d = $%d %s $%d\n", node->index, node->index1, node->type.c_str(), node->index2);
    }
}

void DAG::print_dag_leaf(leaf_dag *leaf) {
    printf("%4d ", ++line_number);
    print_address3(leaf->addr);
}

void tranverse_tree(node_t *node, symbol_table_t *table, DAG *dag) {
    const char *type = node->node_type;
    node_expr *e = (node_expr *)node;
    node_expr *e1 = node->children_num > 0 ? (node_expr *) node->children[0] : nullptr;
    node_expr *e2 = node->children_num > 1 ? (node_expr *) node->children[1] : nullptr;
    node_bool *b = (node_bool *)node;
    node_bool *b1 = node->children_num > 0 ? (node_bool *) node->children[0] : nullptr;
    node_bool *b2 = node->children_num > 2 ? (node_bool *) node->children[2] : nullptr;
    node_flow *s = (node_flow *)node;
    //遍历子结点
    for (int i = 0; i < node->children_num; ++i) {
        if (strcmp(node->children[i]->node_type, "code block") == 0) {
            symbol_table_t *next = next_scope(table);
            tranverse_tree(node->children[i], next, dag);
        } else {
            tranverse_tree(node->children[i], table, dag);
        }
    }
    //执行语义规则
    if (strcmp(type, "expr_const") == 0) { //常量
        dag->try_get_expr(e, "expr_const", atoll(e1->value));
    } else if (strcmp(type, "expr_id") == 0) { //标识符
        symbol_t *symbol = find_symbol(table, node->children[0]->value);
        dag->try_get_expr(e, "expr_id", (long long) symbol);
    } else if (strcmp(type, "=") == 0) { //赋值符号
        e->addr = e1->addr;
        gen_assign(e2->addr, e1->addr);
    } else if (strcmp(type, "+") == 0 ||
               strcmp(type, "-") == 0 ||
               strcmp(type, "*") == 0 ||
               strcmp(type, "/") == 0 ||
               strcmp(type, "%") == 0) {                               //算术运算符
        if (!dag->try_get_expr(e, new node_dag(type, e1->index, e2->index))) {
            gen_binary_op(type, e1->addr, e2->addr, e->addr);
        }
    } else if (strcmp(type, "minus") == 0) { //单目负号
        if (!dag->try_get_expr(e, new node_dag("minus", e1->index))) {
            gen_unary_op(type, e1->addr, e->addr);
        }
    } else if (strcmp(type, "sign_m") == 0) { //控制标记M
        ((node_sign_m *)node)->instr = get_nextinstr();
    } else if (strcmp(type, "||") == 0) { //逻辑或
        node_sign_m *m = (node_sign_m *)node->children[1];
        backpatch(b1->false_list, m->instr);
        b->true_list = merge(b1->true_list, b2->true_list);
        b->false_list = b2->false_list;
    } else if (strcmp(type, "&&") == 0) { //逻辑与
        node_sign_m *m = (node_sign_m *)node->children[1];
        backpatch(b1->true_list, m->instr);
        b->true_list = b2->true_list;
        b->false_list = merge(b1->false_list, b2->false_list);
    } else if (strcmp(type, "!") == 0) { //逻辑非
        b->true_list = b1->false_list;
        b->false_list = b1->true_list;
    } else if (strcmp(type, "==") == 0 ||
               strcmp(type, "!=") == 0 ||
               strcmp(type, ">") == 0 ||
               strcmp(type, "<") == 0 ||
               strcmp(type, ">=") == 0 ||
               strcmp(type, "<=") == 0) {  //关系运算符
        b->true_list = makelist(get_nextinstr());
        b->false_list = makelist(get_nextinstr() + 1);
        gen_if_relop(type, e1->addr, e2->addr, nullptr); //gen if E1.addr rel.op E2.addr goto_
        gen_goto(nullptr); // gen goto_
    } else if (strcmp(type, "expr to bool") == 0) {
        b->true_list = makelist(get_nextinstr());
        b->false_list = makelist(get_nextinstr() + 1);
        gen_if_goto(e1->addr, nullptr);
        gen_goto(nullptr);
    } else if (strcmp(type, "code block") == 0) { //代码块
        if (s->children_num > 0) {
            s->next_list = ((node_flow *)(node->children[0]))->next_list;
        }
    } else if (strcmp(type, "statement list") == 0) { //语句列表
        s->next_list = ((node_flow *)(node->children[0]))->next_list;
        for (int i = 2; i < node->children_num; i += 2) {
            backpatch(s->next_list, ((node_sign_m *)(node->children[i - 1]))->instr);
            s->next_list = ((node_flow *)(node->children[i]))->next_list;
        }
    } else if (strcmp(type, "sign_n") == 0) { //控制标记N
        s->next_list = makelist(get_nextinstr());
        gen_goto(nullptr);
    } else if (strcmp(type, "if statement") == 0) { //if
        b = (node_bool *)s->children[0];
        node_sign_m *m = (node_sign_m *)s->children[1];
        node_flow *s1 = (node_flow *)s->children[2];
        backpatch(b->true_list, m->instr);
        s->next_list = merge(b->false_list, s1->next_list);
    } else if (strcmp(type, "if else statement") == 0) { //if else
        b = (node_bool *)s->children[0];
        node_sign_m *m1 = (node_sign_m *)s->children[1];
        node_sign_m *m2 = (node_sign_m *)s->children[4];
        node_flow *s1 = (node_flow *)s->children[2];
        node_flow *s2 = (node_flow *)s->children[5];
        node_flow *n = (node_flow *)s->children[3];
        backpatch(b->true_list, m1->instr);
        backpatch(b->false_list, m2->instr);
        auto temp = merge(s1->next_list, n->next_list);
        s->next_list = merge(temp, s2->next_list);
    } else if (strcmp(type, "return statement") == 0) { // return
        if(node->children_num == 0) { gen_return(nullptr); }
        else { gen_return(e1->addr); }
    }
}

void get_const_pool(node_t *node, DAG *dag) {
    const char *type = node->node_type;
    for (int i = 0; i < node->children_num; ++i) {
        get_const_pool(node->children[i], dag);
    }
    if (strcmp(type, "expr_const") == 0) { //常量
        node_expr *e = (node_expr *)node;
        node_expr *e1 = (node_expr *) node->children[0];
        dag->try_get_expr(e, "expr_const", atoll(e1->value));
    }
}

void gen_code(node_t *root) {
    symbol_table_t *table = generate_symbol_table(root);
    DAG *dag = new DAG();
    get_const_pool(root, dag);
    tranverse_tree(root, table, dag);
    free_table(table);
    delete dag;
}

void print_quadruple(quadruple *p) {
    QuadrupleType type = get_quadruple_type(p->op);
    if (type == QuadrupleType::BinaryOp ||
        type == QuadrupleType::UnaryOp ||
        type == QuadrupleType::Assign ||
        type == QuadrupleType::Return) {
        printf("%s ", p->op.c_str());
        print_address3(p->arg1);
        print_address3(p->arg2);
        print_address3(p->result);
    } else if (type == QuadrupleType::Goto) {
        printf("goto ");
        if (p->result != nullptr) {
            printf("%lld", p->result->value);
        }
    } else if (type == QuadrupleType::IfGoto) {
        printf("if ");
        print_address3(p->arg1);
        printf("== true goto ");
        if (p->result != nullptr) {
            printf("%lld", p->result->value);
        }
    } else if (type == QuadrupleType::IfRelop) {
        printf("if ");
        print_address3(p->arg1);
        printf("%s ", p->op.c_str());
        print_address3(p->arg2);
        printf(" goto ");
        if (p->result != nullptr) {
            printf("%lld", p->result->value);
        }
    }
    printf("\n");
}

void print_quadruple_array() {
    for (size_t i = 0; i < quadruple_array.size(); ++i) {
        printf("%4ld: ", i);
        print_quadruple(quadruple_array[i]);
    }
}

void print_address3(address3 *address) {
    if (address == nullptr) { return; }
    if (address->type == "num") { printf("#%lld ", address->value); }
    else { printf("$%lld ", address->value); }
}

node_t *new_node_bool(char *node_type, int n, ...) {
    node_bool *node = (node_bool *) malloc(sizeof(node_bool));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = n;
    node->children = (node_t **) malloc(sizeof(node_t *) * n);
    node->true_list = nullptr;
    node->false_list = nullptr;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        node->children[i] = va_arg(ap, node_t*);
    }
    return node;
}

node_t *new_node_expr(char *node_type, int n, ...) {
    node_expr *node = (node_expr *) malloc(sizeof(node_expr));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = n;
    node->children = (node_t **) malloc(sizeof(node_t *) * n);
    node->addr = nullptr;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        node->children[i] = va_arg(ap, node_t*);
    }
    return node;
}

node_t *new_node_sign_m() {
    node_sign_m *node = (node_sign_m*)malloc(sizeof(node_sign_m));
    node->node_type = (char *)"sign_m";
    node->value = NULL;
    node->children_num = 0;
    node->children = NULL;
    node->instr = -1;
    return node;
}

node_t *new_node_flow(char *node_type, int n, ...) {
    node_flow *node = (node_flow*)malloc(sizeof(node_flow));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = n;
    node->children = (node_t **) malloc(sizeof(node_t *) * n);
    node->next_list = nullptr;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        node->children[i] = va_arg(ap, node_t*);
    }
    return node;
}

node_t *new_node_sign_n() {
    return new_node_flow((char *)"sign_n", 0);
}

void gen_binary_op(std::string op, address3* arg1, address3* arg2, address3* result) {
    quadruple_array.push_back(new quadruple(op, arg1, arg2, result));
}

void gen_unary_op(std::string op, address3 *arg1, address3 *result) {
    quadruple_array.push_back(new quadruple(op, arg1, nullptr, result));
}

void gen_assign(address3 *arg1, address3 *result) {
    quadruple_array.push_back(new quadruple("assign", arg1, nullptr, result));
}

void gen_goto(address3 *result) {
    quadruple_array.push_back(new quadruple("goto", nullptr, nullptr, result));
}

void gen_goto(int result) {
    gen_goto(new_address3_number(result));
}

void gen_if_goto(address3 *arg1, address3 *result) {
    quadruple_array.push_back(new quadruple("if_goto", arg1, nullptr, result));
}

void gen_if_relop(std::string op, address3 *arg1, address3 *arg2, address3 *result) {
    quadruple_array.push_back(new quadruple(op, arg1, arg2, result));
}

void gen_return(address3* result) {
    quadruple_array.push_back(new quadruple("return", nullptr, nullptr, result));
}

QuadrupleType get_quadruple_type(std::string op) {
    if (op == "+" ||
        op == "-" ||
        op == "*" ||
        op == "/" ||
        op == "%") {
        return QuadrupleType::BinaryOp;
    } else if (op == "minus") {
        return QuadrupleType::UnaryOp;
    } else if (op == "assign") {
        return QuadrupleType::Assign;
    } else if (op == "goto") {
        return QuadrupleType::Goto;
    } else if (op == "if_goto") {
        return QuadrupleType::IfGoto;
    } else if (op == "==" ||
               op == "!=" ||
               op == ">" ||
               op == "<" ||
               op == ">=" ||
               op == "<=") {
        return QuadrupleType::IfRelop;
    } else if (op == "return") {
        return QuadrupleType::Return;
    }
    return QuadrupleType::NotDefined;
}

std::vector<int> *makelist(int i) {
    std::vector<int> *p = new std::vector<int>();
    p->push_back(i);
    bool_list_pool.push_back(p);
    return p;
}

void backpatch(std::vector<int> *arr, int instr) {
    if (arr == nullptr) { return; }
    for (auto index : *arr) {
        quadruple_array[index]->result = new_address3_number(instr);
    }
}

std::vector<int> *merge(std::vector<int> *arr1, std::vector<int> *arr2) {
    std::vector<int> *p = arr1 == nullptr ? new std::vector<int>() : new std::vector<int>(*arr1);
    if (arr2 != nullptr) {
        p->insert(p->end(), arr2->begin(), arr2->end());
    }
    bool_list_pool.push_back(p);
    return p;
}

address3 *new_address3(std::string type, long long value) {
    address3 *p = new address3(type, value);
    address3_pool.push_back(p);
    return p;
}

address3 *new_address3(std::string type, const char *value) {
    address3 *p = new address3(type, atoll(value));
    address3_pool.push_back(p);
    return p;
}

address3 *new_address3_number(long long number) {
    return new_address3("num", number);
}

address3 *new_address3_number(const char *number) {
    return new_address3("num", number);
}

address3 *new_address3_address(long long address) {
    return new_address3("address", address);
}

address3 *new_address3_address(const char *address) {
    return new_address3("address", address);
}

address3 *new_temp() {
    ++temp_count;
    return new_address3_address(temp_count);
}

int get_nextinstr() {
    return quadruple_array.size();
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

void free_bool_list_pool() {
    for (auto itor = bool_list_pool.begin(); itor != bool_list_pool.end(); ++itor) {
        delete *itor;
    }
}

void free_intermediate_structures() {
    free_address3_pool();
    free_quadruples_array();
    free_bool_list_pool();
}
