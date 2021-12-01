#ifndef COMPILE_INTERMEDIATE_H
#define COMPILE_INTERMEDIATE_H

#include "ast.h"
#include "symbol_table.h"
#include <unordered_map>
#include <string>

//DAG内部结点
struct node_dag { 
    int index; //标号
    std::string type;
    int addr1;
    int addr2;
    node_dag* next;
    node_dag(std::string type, int addr1, int addr2) //对于双目运算符，有两个字段，分别指向左右子结点
        :type(type), addr1(addr1), addr2(addr2), next(nullptr)
        {}
    node_dag(std::string type, int addr1) //单目运算符只有一个子结点
        :type(type), addr1(addr1), addr2(-1), next(nullptr)
        {}
};
//DAG叶子结点
struct leaf_dag { 
    int index;
    std::string type;
    std::string value;//常量的值或标识符的名字
    leaf_dag(std::string type, std::string value) : type(type), value(value) {}
};

class DAG {
public:
    ~DAG();
    DAG() : index_cur(0), line_number(0) {}
    DAG(int offset) : index_cur(offset), line_number(0)  {}
    int get_index(node_dag* new_node);
    int get_index(leaf_dag* new_leaf);
    static bool is_node_equal(node_dag* a, node_dag* b);
    void print_dag_node(node_dag* node);
    void print_dag_leaf(leaf_dag* leaf);
    
    int index_cur; // 当前最大标号
    int line_number;
    std::unordered_map<std::string, node_dag*> node_map; // 内部结点的哈希表存储桶，每个桶是一个链表，存储一小部分表达式结点
    std::unordered_map<std::string, leaf_dag*> leaf_map; // 叶子结点的哈希表存储单个叶子节点
};

void gen_3address_code(node_t *node, symbol_table_t *table, DAG* dag);
void gen_code(node_t *root);


#endif //COMPILE_INTERMEDIATE_H