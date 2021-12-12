#ifndef COMPILE_INTERMEDIATE_H
#define COMPILE_INTERMEDIATE_H

#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include "ast.h"
#include "symbol_table.h"
}

namespace typewidth {
const int INT = 4;
} // namespace typewidth

namespace address3type {
const std::string INT = "int";
const std::string IMMIDIATE = "immidiate"; //立即数
const std::string STRING = "string";
} // namespace address3type

namespace address3typewidth {
const int INT = 12;
} // namespace address3typewidth

enum QuadrupleType {
    BinaryOp,
    UnaryOp,
    Assign,
    Goto,
    IfGoto,
    IfRelop,
    Return,
    NotDefined
};

//地址
struct address3 {
    std::string type;   // address3type
    char *value = NULL; //仅立即数和string有value
    int width;
    long long offset; //立即数无offset

    address3(std::string type, int value) :
        type(type), width(address3typewidth::INT), offset(-1) { //构造int型立即数
        this->value = (char *)malloc(address3typewidth::INT);
        sprintf(this->value, "%d", value);
    }

    address3(std::string type, long long offset) :
        type(type), width(address3typewidth::INT), offset(offset) { //构造int变量
    }

    address3(std::string type, const char *value, long long offset) :
        type(type), offset(offset) { //构造string
        this->value = strdup(value);
        this->width = strlen(value) + 1;
    }
};

//四元式
struct quadruple {
    std::string op;
    address3 *arg1;
    address3 *arg2;
    address3 *result;

    quadruple(std::string op, address3 *arg1, address3 *arg2, address3 *result) :
        op(op), arg1(arg1), arg2(arg2), result(result) {
    }
};

//表达式结点E
struct node_expr : node_t {
    address3 *addr;
    int index; // DAG结点标号
};

//布尔表达式结点B
struct node_bool : node_t {
    std::vector<int> *true_list;
    std::vector<int> *false_list;
};

//控制流标记结点M
struct node_sign_m : node_t {
    int instr;
};

struct node_flow : node_t {
    std::vector<int> *next_list;
};

// DAG内部结点
struct node_dag {
    int index; //标号
    std::string type;
    int index1;     //左子标号
    int index2;     //右子标号
    address3 *addr; //存储表达式值的临时变量地址
    node_dag *next;

    node_dag(std::string type, int index1, int index2) :
        index(-1),
        type(type),
        index1(index1),
        index2(index2),
        addr(nullptr),
        next(nullptr) { //双目运算符有两个子结点
    }

    node_dag(std::string type, int index1) :
        index(-1),
        type(type),
        index1(index1),
        index2(-1),
        addr(nullptr),
        next(nullptr) { //单目运算符只有一个子结点
    }
};

// DAG叶子结点
struct leaf_dag {
    int index;
    address3 *addr;

    leaf_dag() :
        index(-1), addr(nullptr) {
    }

    leaf_dag(address3 *addr) :
        index(-1), addr(addr) {
    }
};

class DAG {
public:
    ~DAG();
    DAG() :
        index_cur(0), line_number(0) {
    }
    DAG(int offset) :
        index_cur(offset), line_number(0) {
    }
    bool try_get_expr(node_expr *e, node_dag *new_node);                      //尝试匹配公共子表达式
    bool try_get_const_int(node_expr *e, std::string type, int value);        //尝试匹配单个常量
    bool try_get_variable(node_expr *e, std::string type, long long &offset); //尝试匹配单个变量
    static bool is_node_equal(node_dag *a, node_dag *b);
    void print_dag_node(node_dag *node);
    void print_dag_leaf(leaf_dag *leaf);

    int index_cur; // 当前最大标号
    int line_number;
    std::unordered_map<std::string, node_dag *>
        node_map; // 内部结点的哈希表存储桶，每个桶是一个链表，存储一小部分表达式结点
    std::unordered_map<std::string, leaf_dag *>
        leaf_map; // 叶子结点的哈希表存储单个叶子节点
};

void tranverse_tree(node_t *node, symbol_table_t *table, DAG *dag);
void get_const_pool(node_t *node, DAG *dag);

extern "C" {
void gen_code(node_t *root);
void print_quadruple_array();
void print_type_warning();
void free_intermediate_structures();
node_t *new_node_bool(char *node_type, int n, ...);
node_t *new_node_expr(char *node_type, int n, ...);
node_t *new_node_sign_m();
node_t *new_node_flow(char *node_type, int n, ...);
node_t *new_node_sign_n();
void print_raw_tree(node_t *node, int depth);
}

void print_address3(address3 *address);
void print_address3_value(address3 *address);
void print_quadruple(quadruple *p);
void gen_binary_op(std::string op, address3 *arg1, address3 *arg2, address3 *result); // result = arg1 op arg2
void gen_unary_op(std::string op, address3 *arg1, address3 *result);                  // result = op arg1
void gen_assign(address3 *arg1, address3 *result);                                    // result = arg1
void gen_goto(address3 *result);                                                      // goto result(num)
void gen_goto(int result);                                                            // goto result(num)
void gen_if_goto(address3 *arg1, address3 *result);                                   // if (arg1 == true) goto result
void gen_if_relop(std::string op, address3 *arg1, address3 *arg2, address3 *result);  // if (arg1 rel.op arg2) goto result
void gen_return(address3 *result);
QuadrupleType get_quadruple_type(std::string op);

std::vector<int> *makelist(int i);
void backpatch(std::vector<int> *arr, int instr);
std::vector<int> *merge(std::vector<int> *arr1, std::vector<int> *arr2);

address3 *new_address3(std::string type, int value);
address3 *new_address3(std::string type, long long offset);
address3 *new_address3(std::string type, const char *value, long long offset);
address3 *new_address3_immidiate(int value);
address3 *new_address3_int(long long offset);
address3 *new_address3_string(const char *value, long long offset);
address3 *new_temp_int();
address3 *new_temp_string(const char *value);
int get_nextinstr();

void print_type_warning();
void free_address3_pool();
void free_quadruples_array();
void free_bool_list_pool();

#endif // COMPILE_INTERMEDIATE_H