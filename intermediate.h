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
const int LONGLONG = 8;
} // namespace typewidth

namespace address3type {
const std::string INT = "int";
const std::string STRING = "string";
const std::string INT_VALUE = "int_value"; //立即数(值类型)
const std::string STRING_VALUE = "string_value";
const std::string LONGLONG = "longlong";
} // namespace address3type

namespace nonterminal_symbol_type {
const std::string CINT = "const:int";
const std::string CSTRING = "const:string";
} // namespace nonterminal_symbol_type

enum QuadrupleType { BinaryOp, UnaryOp, Assign, Goto, IfGoto, IfRelop, Return, Param, Call, NotDefined };

//地址
struct address3 {
    std::string type;      // address3type
    char *value = nullptr; //仅值类型有value
    long long offset;      //仅变量类型有offset
    int width;             //仅变量类型有width

    address3(std::string type, const char *value) : type(type), offset(-1), width(0) { //构造值类型
        this->value = new char[strlen(value) + 1]();
        strcpy(this->value, value);
    }

    address3(std::string type, long long offset, int width) : type(type), offset(offset), width(width) { //构造变量类型
    }
};

//四元式
struct quadruple {
    std::string op;
    address3 *arg1;
    address3 *arg2;
    address3 *result;

    quadruple(std::string op, address3 *arg1, address3 *arg2, address3 *result) : op(op), arg1(arg1), arg2(arg2), result(result) {
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
        index(-1), type(type), index1(index1), index2(index2), addr(nullptr), next(nullptr) { //双目运算符有两个子结点
    }

    node_dag(std::string type, int index1) :
        index(-1), type(type), index1(index1), index2(-1), addr(nullptr), next(nullptr) { //单目运算符只有一个子结点
    }
};

// DAG叶子结点
struct leaf_dag {
    int index;
    address3 *addr;

    leaf_dag() : index(-1), addr(nullptr) {
    }

    leaf_dag(address3 *addr) : index(-1), addr(addr) {
    }
};

class DAG {
public:
    ~DAG();
    DAG() : index_cur(0), line_number(0) {
    }
    DAG(int offset) : index_cur(offset), line_number(0) {
    }
    bool try_get_expr(node_expr *e, node_dag *new_node, std::string addr_type);                      //尝试匹配公共子表达式
    bool try_get_const(node_expr *e, std::string type, const char *value);                           //尝试匹配单个常量
    bool try_get_variable(node_expr *e, std::string type, long long &offset, std::string addr_type); //尝试匹配单个变量
    void create_value_and_assign(node_expr *e, std::string type, const char *value);
    void print_dag_node(node_dag *node);
    void print_dag_leaf(leaf_dag *leaf);
    static bool is_node_equal(node_dag *a, node_dag *b);

    int index_cur; // 当前最大标号
    int line_number;
    std::unordered_map<std::string, node_dag *> node_map; // 内部结点的哈希表存储桶，每个桶是一个链表，存储一小部分表达式结点
    std::unordered_map<std::string, leaf_dag *> leaf_map; // 叶子结点的哈希表存储单个叶子节点
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
void gen_return(address3 *result);                                                    // return result
void gen_param(address3 *result);                                                     // param result
void gen_call(address3 *arg1, address3 *arg2);                                        // call arg1(函数名) arg2(实参个数)
QuadrupleType get_quadruple_type(std::string op);

std::vector<int> *makelist(int i);
void backpatch(std::vector<int> *arr, int instr);
std::vector<int> *merge(std::vector<int> *arr1, std::vector<int> *arr2);
int get_nextinstr();

address3 *new_address3(std::string type, const char *value);
address3 *new_address3(std::string type, long long offset, int width);
address3 *new_address3_int_value(const char *value);
address3 *new_address3_int_value(int value);
address3 *new_address3_string_value(const char *value);
address3 *new_address3_int(long long offset);
address3 *new_address3_string(long long offset, int width);
address3 *new_address3_longlong(long long offset);
address3 *new_temp_int();
address3 *new_temp_string(int length);
address3 *new_temp_longlong();
address3 *new_temp(std::string type); //根据type生成非string类型的临时变量

void print_type_warning();
void free_address3_pool();
void free_quadruples_array();
void free_bool_list_pool();

#endif // COMPILE_INTERMEDIATE_H