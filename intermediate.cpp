#include "intermediate.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

extern FILE *ast_out;
std::vector<quadruple *> quadruple_array;
std::vector<address3 *> address3_pool;
std::vector<std::vector<int> *> bool_list_pool;
long long offset_global = 0;
char type_warning[1024] = {'\0'};
long long const_string_offset = 0;
const std::string *environment_address_type =
    &address3type::INT; //默认采用32位地址

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

bool DAG::try_get_expr(node_expr *e, node_dag *new_node,
                       std::string addr_type) {
    int index = -1;
    bool found = false;
    auto got = node_map.find(new_node->type);
    if (got == node_map.end()) { // 匹配桶失败，新建桶
        new_node->addr = e->addr = new_temp(addr_type); // 新建int型临时变量
        new_node->index = index = ++index_cur;
        // node_map.emplace(new_node->type, new_node);
        delete new_node;
    } else {
        node_dag *head = got->second;
        while (head != nullptr) {
            if (is_node_equal(head, new_node)) { // 匹配成功
                e->addr = head->addr;            // 复制地址
                index = head->index;             // 复制标号
                found = true;
                delete new_node; // 不需要新结点，释放内存
                break;
            } else if (head->next == nullptr) { // 匹配失败，将新结点加入链表
                new_node->addr = e->addr = new_temp(addr_type); // 新建临时变量
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

bool DAG::try_get_const(node_expr *e, std::string type, const char *value) {
    int index = -1;
    bool found = false;
    std::string key = type + std::string(value);
    auto got = leaf_map.find(key);
    if (got == leaf_map.end()) { // 匹配失败，插入新结点
        create_value_and_assign(e, type, value);
        leaf_dag *new_leaf = new leaf_dag(e->addr);
        new_leaf->index = index = ++index_cur;
        leaf_map.emplace(key, new_leaf);
    } else {                         //匹配成功
        e->addr = got->second->addr; // 复制地址
        index = got->second->index;  // 复制标号
        found = true;
    }
    e->index = index;
    return found;
}

bool DAG::try_get_variable(node_expr *e, std::string type,
                           long long &symbol_offset, std::string addr_type) {
    int index = -1;
    bool found = false;
    if (symbol_offset == -1) {                //已声明，未分配内存
        symbol_offset = offset_global;        //在符号表中记录offset
        address3 *addr = new_temp(addr_type); //分配int型内存
        e->addr = addr;
        std::string key = type + std::to_string(symbol_offset);
        leaf_dag *new_leaf = new leaf_dag(addr);
        new_leaf->index = index = ++index_cur;
        auto got = leaf_map.find(key);
        if (got == leaf_map.end()) { //不存在结点，新建结点
            leaf_map.emplace(key, new_leaf);
        } else { //覆盖旧结点
            delete got->second;
            got->second = new_leaf;
        }
    } else {
        std::string key = type + std::to_string(symbol_offset);
        auto got = leaf_map.find(key);
        e->addr = got->second->addr; // 复制地址
        index = got->second->index;  // 复制标号
        found = true;
    }
    e->index = index;
    return found;
}

void DAG::create_value_and_assign(node_expr *e, std::string type,
                                  const char *value) {
    address3 *addr = nullptr;
    if (type == nonterminal_symbol_type::CSTRING) {
        addr = new_const_string(value);
        gen_string(new_address3_string_value(value), addr);
    } else if (type == nonterminal_symbol_type::CINT) {
        addr = new_temp_int();
        gen_assign(new_address3_int_value(value), addr);
    } else { // undifined behaviour!
        addr = new_temp_int();
        gen_assign(new_address3_int_value("undifined"), addr);
    }
    e->addr = addr;
}

bool DAG::is_node_equal(node_dag *a, node_dag *b) {
    return a->type == b->type && a->index1 == b->index1 &&
           a->index2 == b->index2;
}

void DAG::print_dag_node(node_dag *node) {
    printf("%4d ", ++line_number);
    if (node->type == "minus") {
        printf("$%d = %s $%d\n", node->index, node->type.c_str(), node->index1);
    } else if (node->type == "=") {
        printf("$%d %s $%d\n", node->index1, node->type.c_str(), node->index2);
    } else {
        printf("$%d = $%d %s $%d\n", node->index, node->index1,
               node->type.c_str(), node->index2);
    }
}

void DAG::print_dag_leaf(leaf_dag *leaf) {
    printf("%4d ", ++line_number);
    print_address3(leaf->addr);
}

void tranverse_tree(node_t *node, symbol_table_t *table, DAG *dag) {
    const char *type = node->node_type;
    node_expr *e = (node_expr *)node;
    node_expr *e1 =
        node->children_num > 0 ? (node_expr *)node->children[0] : nullptr;
    node_expr *e2 =
        node->children_num > 1 ? (node_expr *)node->children[1] : nullptr;
    node_bool *b = (node_bool *)node;
    node_bool *b1 =
        node->children_num > 0 ? (node_bool *)node->children[0] : nullptr;
    node_bool *b2 =
        node->children_num > 2 ? (node_bool *)node->children[2] : nullptr;
    node_flow *s = (node_flow *)node;
    if (strcmp(type, "declare clause") == 0) {
        symbol_t symbol;
        symbol.type = strdup(node->children[0]->node_type);
        symbol.name = node->children[0]->value;
        symbol.offset = -1; //未分配地址
        insert_symbol(table, symbol, node->line);
    }
    //遍历子结点
    for (int i = 0; i < node->children_num; ++i) {
        if (strcmp(node->children[i]->node_type, "code block") == 0 ||
            strcmp(node->children[i]->node_type, "for statement") == 0) {
            long long pre_offset = offset_global;    //记录当前栈顶
            symbol_table_t *next = new_scope(table); //创建新作用域
            tranverse_tree(node->children[i], next, dag);
            offset_global = pre_offset; //回退栈顶值
        } else {
            tranverse_tree(node->children[i], table, dag);
        }
    }
    //执行语义规则
    if (strcmp(type, "expr_const") == 0) { //常量
        const node_t *child = node->children[0];
        dag->try_get_const(e, child->node_type, child->value);
        // char tmp[100] = {'\0'};
        // if (strchr(e1->value, '.')) {
        //     sprintf(tmp, "line %d: float to int\n", e->line);
        //     strcat(type_warning, tmp);
        // }
    } else if (strcmp(type, "expr_id") == 0) { //标识符
        char tmp[100];
        sprintf(tmp, "%s%s%d", node->children[0]->value, "?line ", e->line);
        symbol_t *symbol = find_symbol(table, tmp);
        dag->try_get_variable(e, node->children[0]->node_type, symbol->offset,
                              address3type::INT);
    } else if (strcmp(type, "=") == 0) { //赋值符号
        e->addr = e1->addr;
        gen_assign(e2->addr, e1->addr);
    } else if (strcmp(type, "+") == 0 || strcmp(type, "-") == 0 ||
               strcmp(type, "*") == 0 || strcmp(type, "/") == 0 ||
               strcmp(type, "%") == 0) { //双目运算符
        if (!dag->try_get_expr(e, new node_dag(type, e1->index, e2->index),
                               address3type::INT)) {
            gen_binary_op(type, e1->addr, e2->addr, e->addr);
        }
    } else if (strcmp(type, "minus") == 0 ||
               strcmp(type, "&") == 0) { //单目运算符
        std::string addr_type = strcmp(type, "&") == 0
                                    ? (*environment_address_type)
                                    : address3type::INT;
        if (!dag->try_get_expr(e, new node_dag(type, e1->index), addr_type)) {
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
    } else if (strcmp(type, "==") == 0 || strcmp(type, "!=") == 0 ||
               strcmp(type, ">") == 0 || strcmp(type, "<") == 0 ||
               strcmp(type, ">=") == 0 ||
               strcmp(type, "<=") == 0) { //关系运算符
        b->true_list = makelist(get_nextinstr());
        b->false_list = makelist(get_nextinstr() + 1);
        gen_if_relop(type, e1->addr, e2->addr,
                     nullptr); // gen if E1.addr rel.op E2.addr goto_
        gen_goto(nullptr);     // gen goto_
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
            backpatch(s->next_list,
                      ((node_sign_m *)(node->children[i - 1]))->instr);
            s->next_list = ((node_flow *)(node->children[i]))->next_list;
        }
    } else if (strcmp(type, "sign_n") == 0) { //控制标记N
        s->next_list = makelist(get_nextinstr());
        gen_goto(nullptr);
    } else if (strcmp(type, "if statement") == 0) { // if
        b = (node_bool *)s->children[0];
        node_sign_m *m = (node_sign_m *)s->children[1];
        node_flow *s1 = (node_flow *)s->children[2];
        backpatch(b->true_list, m->instr);
        s->next_list = merge(b->false_list, s1->next_list);
    } else if (strcmp(type, "if else statement") == 0) { // if else
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
        if (node->children_num == 0) {
            gen_return(nullptr);
        } else {
            gen_return(e1->addr);
        }
    } else if (strcmp(type, "for statement") == 0) { // for
        node_bool *for_b = (node_bool *)node->children[2];
        node_flow *for_s = (node_flow *)node->children[7];
        node_sign_m *m1 = (node_sign_m *)node->children[1];
        node_sign_m *m2 = (node_sign_m *)node->children[3];
        node_sign_m *m3 = (node_sign_m *)node->children[6];
        node_flow *n1 = (node_flow *)node->children[5];
        backpatch(for_b->true_list, m3->instr);
        backpatch(n1->next_list, m1->instr);
        backpatch(for_s->next_list, m2->instr);
        gen_goto(m2->instr);
        s->next_list = for_b->false_list;
    } else if (strcmp(type, "while statement") == 0) { // while
        b = (node_bool *)s->children[1];
        node_sign_m *m1 = (node_sign_m *)s->children[0];
        node_sign_m *m2 = (node_sign_m *)s->children[2];
        node_flow *s1 = (node_flow *)s->children[3];
        backpatch(s1->next_list, m1->instr);
        backpatch(b->true_list, m2->instr);
        s->next_list = b->false_list;
        gen_goto(m1->instr);
    } else if (strcmp(type, "do while statement") == 0) { // do while
        b = (node_bool *)s->children[3];
        node_sign_m *m1 = (node_sign_m *)s->children[0];
        node_sign_m *m2 = (node_sign_m *)s->children[2];
        node_flow *s1 = (node_flow *)s->children[1];
        backpatch(s1->next_list, m2->instr);
        backpatch(b->true_list, m1->instr);
        s->next_list = b->false_list;
    } else if (strcmp(type, "call arguments") == 0) {
        node_expr *ex = nullptr;
        for (int i = 0; i < node->children_num; ++i) {
            ex = (node_expr *)node->children[i];
            gen_param(ex->addr);
        }
    } else if (strcmp(type, "call function") == 0) {
        gen_call(new_address3_string_value(node->children[0]->value),
                 new_address3_int_value(node->children[1]->children_num));
    } else if (strcmp(type, "left_auto_incr") == 0) {
        gen_binary_op("+", e1->addr, new_address3_int_value(1), e1->addr);
    } else if (strcmp(type, "left_auto_decr") == 0) {
        gen_binary_op("-", e1->addr, new_address3_int_value(1), e1->addr);
    }
}

void get_const_pool(node_t *node, DAG *dag) {
    const char *type = node->node_type;
    for (int i = 0; i < node->children_num; ++i) {
        get_const_pool(node->children[i], dag);
    }
    if (strcmp(type, "expr_const") == 0) { //常量
        node_expr *e = (node_expr *)node;
        dag->try_get_const(e, node->children[0]->node_type,
                           node->children[0]->value);
    }
}

void get_const_string_pool(node_t *node, DAG *dag) {
    const char *type = node->node_type;
    for (int i = 0; i < node->children_num; ++i) {
        get_const_string_pool(node->children[i], dag);
    }
    if (strcmp(type, "expr_const") == 0 &&
        strcmp(node->children[0]->node_type, "const:string") ==
            0) { // string常量
        node_expr *e = (node_expr *)node;
        dag->try_get_const(e, node->children[0]->node_type,
                           node->children[0]->value);
    }
}

void gen_code(node_t *root) {
    //如果需要设置64位编译环境，在调用gen_code前先调用set_environment_64bit
    symbol_table_t *table = new_scope(NULL);
    DAG *dag = new DAG();
    get_const_string_pool(root, dag);
    get_const_pool(root, dag);
    tranverse_tree(root, table, dag);
    free_table(table);
    delete dag;
}

void print_quadruple(quadruple *p) {
    QuadrupleType type = get_quadruple_type(p->op);
    if (type == QuadrupleType::BinaryOp || type == QuadrupleType::UnaryOp ||
        type == QuadrupleType::Assign || type == QuadrupleType::Return ||
        type == QuadrupleType::Param || type == QuadrupleType::Call ||
        type == QuadrupleType::String || type == QuadrupleType::Goto) {
        printf("%s ", p->op.c_str());
        print_address3(p->arg1);
        print_address3(p->arg2);
        print_address3(p->result);
    } else if (type == QuadrupleType::IfGoto) {
        printf("if ");
        print_address3(p->arg1);
        printf("== true goto ");
        print_address3(p->arg2);
        print_address3_value(p->result);
    } else if (type == QuadrupleType::IfRelop) {
        printf("if ");
        print_address3(p->arg1);
        printf("%s ", p->op.c_str());
        print_address3(p->arg2);
        printf("goto ");
        print_address3_value(p->result);
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
    if (address == nullptr) {
        printf("__ ");
    } else if (address->type == address3type::INT ||
               address->type == address3type::LONGLONG) {
        printf("(%s $%lld, %d) ", address->type.c_str(), address->offset,
               address->width);
    } else if (address->type == address3type::INT_VALUE ||
               address->type == address3type::STRING_VALUE) {
        printf("(%s #%s) ", address->type.c_str(), address->value);
    } else if (address->type == address3type::STRING) {
        printf("(%s, v%lld) ", address->type.c_str(), address->offset);
    } else {
        printf("type=%s width=%d offset=%lld\n", address->type.c_str(),
               address->width, address->offset);
    }
}

void print_address3_value(address3 *address) {
    if (address == nullptr) {
        return;
    }
    printf("%s", address->value);
}

node_t *new_node_bool(char *node_type, int n, ...) {
    node_bool *node = (node_bool *)malloc(sizeof(node_bool));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = n;
    node->children = (node_t **)malloc(sizeof(node_t *) * n);
    node->true_list = nullptr;
    node->false_list = nullptr;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        node->children[i] = va_arg(ap, node_t *);
    }
    return node;
}

node_t *new_node_expr(char *node_type, int n, ...) {
    int m = n;
    if (strstr(node_type, "expr")) {
        m = n - 1;
    }
    node_expr *node = (node_expr *)malloc(sizeof(node_expr));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = m;
    node->children = (node_t **)malloc(sizeof(node_t *) * m);
    node->addr = nullptr;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        if (strstr(node_type, "expr") && i == 1) {
            node->line = va_arg(ap, int);
        } else {
            node->children[i] = va_arg(ap, node_t *);
        }
    }
    return node;
}

node_t *new_node_sign_m() {
    node_sign_m *node = (node_sign_m *)malloc(sizeof(node_sign_m));
    node->node_type = (char *)"sign_m";
    node->value = NULL;
    node->children_num = 0;
    node->children = NULL;
    node->instr = -1;
    return node;
}

node_t *new_node_flow(char *node_type, int n, ...) {
    node_flow *node = (node_flow *)malloc(sizeof(node_flow));
    node->node_type = node_type;
    node->value = NULL;
    node->children_num = n;
    node->children = (node_t **)malloc(sizeof(node_t *) * n);
    node->next_list = nullptr;
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
        node->children[i] = va_arg(ap, node_t *);
    }
    return node;
}

node_t *new_node_sign_n() { return new_node_flow((char *)"sign_n", 0); }

void print_raw_tree(node_t *node, int depth) {
    for (int i = 0; i < depth; ++i) {
        fprintf(ast_out, "| ");
    }
    printf("|-(%s) %d", node->node_type, node->children_num);
    if (strncmp(node->node_type, "const", 5) == 0 ||
        strcmp(node->node_type, "id") == 0) {
        printf(" [%s]", node->value);
    }
    printf("\n");
    for (int i = 0; i < node->children_num; ++i) {
        print_raw_tree(node->children[i], depth + 1);
    }
}

void gen_binary_op(std::string op, address3 *arg1, address3 *arg2,
                   address3 *result) {
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

void gen_goto(int result) { gen_goto(new_address3_int_value(result)); }

void gen_if_goto(address3 *arg1, address3 *result) { //转为if_relop
    quadruple_array.push_back(
        new quadruple("!=", arg1, new_address3_int_value(0), result));
}

void gen_if_relop(std::string op, address3 *arg1, address3 *arg2,
                  address3 *result) {
    quadruple_array.push_back(new quadruple(op, arg1, arg2, result));
}

void gen_return(address3 *result) {
    quadruple_array.push_back(
        new quadruple("return", nullptr, nullptr, result));
}

void gen_param(address3 *result) {
    quadruple_array.push_back(new quadruple("param", nullptr, nullptr, result));
}

void gen_call(address3 *arg1, address3 *arg2) {
    quadruple_array.push_back(new quadruple("call", arg1, arg2, nullptr));
}

void gen_string(address3 *arg1, address3 *result) {
    quadruple_array.push_back(new quadruple("string", arg1, nullptr, result));
}

QuadrupleType get_quadruple_type(std::string op) {
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
        return QuadrupleType::BinaryOp;
    } else if (op == "minus" || op == "&") {
        return QuadrupleType::UnaryOp;
    } else if (op == "assign") {
        return QuadrupleType::Assign;
    } else if (op == "goto") {
        return QuadrupleType::Goto;
    } else if (op == "if_goto") {
        return QuadrupleType::IfGoto;
    } else if (op == "==" || op == "!=" || op == ">" || op == "<" ||
               op == ">=" || op == "<=") {
        return QuadrupleType::IfRelop;
    } else if (op == "return") {
        return QuadrupleType::Return;
    } else if (op == "param") {
        return QuadrupleType::Param;
    } else if (op == "call") {
        return QuadrupleType::Call;
    } else if (op == "string") {
        return QuadrupleType::String;
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
    if (arr == nullptr) {
        return;
    }
    for (auto index : *arr) {
        quadruple_array[index]->result = new_address3_int_value(instr);
    }
}

std::vector<int> *merge(std::vector<int> *arr1, std::vector<int> *arr2) {
    std::vector<int> *p =
        arr1 == nullptr ? new std::vector<int>() : new std::vector<int>(*arr1);
    if (arr2 != nullptr) {
        p->insert(p->end(), arr2->begin(), arr2->end());
    }
    bool_list_pool.push_back(p);
    return p;
}

int get_nextinstr() { return quadruple_array.size(); }

address3 *new_address3(std::string type, const char *value) {
    address3 *p = new address3(type, value);
    address3_pool.push_back(p);
    return p;
}

address3 *new_address3(std::string type, long long offset, int width) {
    address3 *p = new address3(type, offset, width);
    address3_pool.push_back(p);
    return p;
}

address3 *new_address3_int_value(const char *value) {
    return new_address3(address3type::INT_VALUE, value);
}

address3 *new_address3_int_value(int value) {
    char *value_str = new char[21];
    sprintf(value_str, "%d", value);
    address3 *p = new_address3(address3type::INT_VALUE, value_str);
    delete value_str;
    return p;
}

address3 *new_address3_string_value(const char *value) {
    return new_address3(address3type::STRING_VALUE, value);
}

address3 *new_address3_int(long long offset) {
    return new_address3(address3type::INT, offset, typewidth::INT);
}

address3 *new_address3_string(long long offset, int width) {
    return new_address3(address3type::STRING, offset, width);
}

address3 *new_address3_longlong(long long offset) {
    return new_address3(address3type::LONGLONG, offset, typewidth::LONGLONG);
}

address3 *new_temp_int() {
    address3 *p = new_address3_int(offset_global);
    offset_global += typewidth::INT;
    return p;
}

address3 *new_temp_string(int length) {
    length += 1;
    address3 *p = new_address3_string(offset_global, length);
    offset_global +=
        length % 4 == 0 ? length : (length / 4 + 1) * 4; //保持地址为4的倍数
    return p;
}

address3 *new_temp_longlong() {
    address3 *p = new_address3_longlong(offset_global);
    offset_global += typewidth::LONGLONG;
    return p;
}

address3 *new_temp(std::string type) {
    if (type == address3type::INT) {
        return new_temp_int();
    } else if (type == address3type::LONGLONG) {
        return new_temp_longlong();
    }
    return new_temp_int(); // undifined behaviour!
}

address3 *new_const_string(const char *value) {
    address3 *p = new address3(address3type::STRING, value);
    p->offset = ++const_string_offset;
    address3_pool.push_back(p);
    return p;
}

void print_type_warning() { printf("%s", type_warning); }

void free_address3_pool() {
    for (auto itor = address3_pool.begin(); itor != address3_pool.end();
         ++itor) {
        if ((*itor)->value != nullptr) {
            delete (*itor)->value;
        }
        delete *itor;
    }
}

void free_quadruples_array() {
    for (auto itor = quadruple_array.begin(); itor != quadruple_array.end();
         ++itor) {
        delete *itor;
    }
}

void free_bool_list_pool() {
    for (auto itor = bool_list_pool.begin(); itor != bool_list_pool.end();
         ++itor) {
        delete *itor;
    }
}

void free_intermediate_structures() {
    free_address3_pool();
    free_quadruples_array();
    free_bool_list_pool();
}

void set_environment_32bit() { environment_address_type = &address3type::INT; }

void set_environment_64bit() {
    environment_address_type = &address3type::LONGLONG;
}
