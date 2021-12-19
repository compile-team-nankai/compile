#ifndef ASM_GENERATOR_H
#define ASM_GENERATOR_H
#include "intermediate.h"
#include <iostream>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace instruction {
const std::string mov = "mov";
const std::string add = "add";
const std::string sub = "sub";
const std::string imul = "imul";
const std::string idiv = "idiv";

const std::string je = "je";
const std::string jne = "jne";
const std::string ja = "ja";
const std::string jae = "jae";
const std::string jb = "jb";
const std::string jbe = "jbe";
const std::string jmp = "jmp";

const std::string cmp = "cmp";

const std::string push = "push";
const std::string pop = "pop";
const std::string call = "call";
const std::string cdq = "cdq";
} // namespace instruction

namespace line {
class line;
}
namespace operand {
class operand {
  public:
    virtual std::string to_string() = 0;
    static operand *get_operand(address3 *, int &, std::vector<line::line *> &);
};
class immediate : public operand {
    long long num;

  public:
    immediate(char *num);
    immediate(long long num);
    std::string to_string();
};
class reg : public operand {
    std::string name;

  public:
    reg(std::string name);
    std::string to_string();
};

class memory : public operand {
    std::string type = "dword";
    int offset;

  public:
    memory(int offset, std::string type = "dword");
    std::string to_string();
};
class label : public operand {
    std::string var;

  public:
    label(std::string var);
    std::string to_string();
};
} // namespace operand

namespace line {
class line {
  public:
    virtual std::string to_string() = 0;
};
class instruction : public line {
    std::string instructor;
    std::vector<operand::operand *> params;

  public:
    instruction(std::string instructor, std::vector<operand::operand *> params);
    ~instruction();
    std::string to_string();
};
class label : public line {
    std::string var;

  public:
    label(std::string var);
    std::string to_string();
    std::string get_var();
};
class variable : public line {
    std::string name;
    std::string value;

  public:
    variable(std::string name, std::string value);
    std::string to_string();
};
class extern_ : public line {
    std::string function;

  public:
    extern_(std::string);
    std::string to_string();
};
class section : public line {
    std::string name;

  public:
    section(std::string);
    std::string to_string();
};
} // namespace line

class Asm {
    line::label global;
    std::vector<line::variable *> variables;
    std::vector<line::line *> instructions;
    std::vector<line::extern_ *> externs;
    std::unordered_set<int> labels;
    int stack_size = 0;
    void add_handler(quadruple);
    void sub_handler(quadruple);
    void mul_handler(quadruple);
    void div_handler(quadruple);
    void mod_handler(quadruple);
    void j_handler(quadruple);
    void jmp_handler(quadruple);
    void call_handler(quadruple);
    void param_handler(quadruple);
    void assign_handler(quadruple);
    void string_handler(quadruple);
    void address_handler(quadruple);
    void return_handler(quadruple);

    const std::unordered_map<std::string, void (Asm::*)(quadruple)> handlers = {
        {"+", &Asm::add_handler},         {"-", &Asm::sub_handler},
        {"*", &Asm::mul_handler},         {"/", &Asm::div_handler},
        {"%", &Asm::mod_handler},         {"==", &Asm::j_handler},
        {"!=", &Asm::j_handler},          {">", &Asm::j_handler},
        {"<", &Asm::j_handler},           {">=", &Asm::j_handler},
        {"<=", &Asm::j_handler},          {"goto", &Asm::jmp_handler},
        {"call", &Asm::call_handler},     {"param", &Asm::param_handler},
        {"assign", &Asm::assign_handler}, {"string", &Asm::string_handler},
        {"&", &Asm::address_handler},     {"return", &Asm::return_handler}};
    const std::unordered_map<std::string, std::string> get_condition_jmp = {
        {"==", instruction::je},  {"!=", instruction::jne},
        {">", instruction::ja},   {"<", instruction::jb},
        {">=", instruction::jae}, {"<=", instruction::jbe},
    };

  public:
    Asm(line::label global = line::label("main"),
        std::vector<line::extern_ *> externs = {new line::extern_("printf"),
                                                new line::extern_("scanf"),
                                                new line::extern_("exit")

        });
    std::vector<std::string> to_lines();
    void handle(std::vector<quadruple *>);
    ~Asm();
};

void to_asm(std::vector<quadruple> quadruples);

namespace reg {
const operand::reg eax = std::string("eax");
const operand::reg ebx = std::string("ebx");
const operand::reg ecx = std::string("ecx");
const operand::reg edx = std::string("edx");
const operand::reg esp = std::string("esp");
const operand::reg ebp = std::string("ebp");
const operand::reg esi = std::string("esi");
const operand::reg edi = std::string("edi");
} // namespace reg

#endif
