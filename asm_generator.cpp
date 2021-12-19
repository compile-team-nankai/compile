#include "asm_generator.h"
using namespace std;

operand::immediate::immediate(char *num) { this->num = atoi(num); }
operand::immediate::immediate(long long num) { this->num = num; }
operand::label::label(string var) { this->var = var; }
operand::memory::memory(int offset, std::string type) {
    this->offset = offset;
    this->type = type;
}
operand::reg::reg(string name) { this->name = name; }

line::instruction::instruction(string instructor,
                               vector<operand::operand *> params) {
    this->instructor = instructor;
    this->params = params;
}
line::instruction::~instruction() {
    for (auto i : params) {
        delete i;
    }
}

line::label::label(string var) { this->var = var; }
line::variable::variable(string name, string value) {
    this->name = name;
    this->value = value;
}
line::extern_::extern_(string name) { this->function = name; }
line::section::section(string name) { this->name = name; }

string operand::immediate::to_string() { return std::to_string(this->num); }
string operand::label::to_string() { return this->var; }
string operand::memory::to_string() {
    return this->type + " [ebp-" + std::to_string(this->offset + 4) + "]";
}
string operand::reg::to_string() { return this->name; }

extern void stack_resize(int &stack_size, long long offset,
                         std::vector<line::line *> &lines);

operand::operand *
operand::operand::get_operand(address3 *addr, int &stack_size,
                              std::vector<line::line *> &lines) {
    if (addr->type == "int_value") {
        return new immediate(addr->value);
    } else if (addr->type == "int") {
        stack_resize(stack_size, addr->offset + addr->width, lines);
        return new memory(addr->offset);
    } else { //  if (addr->type == "string")
        stack_resize(stack_size, addr->offset + addr->width, lines);
        return new label(string("v") + std::to_string(addr->offset));
    }
}

string line::instruction::to_string() {
    string params = "";

    for (int i = 0; i < this->params.size(); ++i) {
        params += this->params[i]->to_string();
        if (i != this->params.size() - 1)
            params += ",";
    }
    return this->instructor + " " + params;
}
string line::label::to_string() { return this->var + ":"; }
string line::variable::to_string() {
    // this->value = this->value.substr(1, this->value.length() - 2);
    return this->name + " db `" + this->value + "`,0";
}
string line::extern_::to_string() { return "extern " + this->function; }
string line::section::to_string() { return "section ." + this->name; }

string line::label::get_var() { return this->var; }

Asm::Asm(line::label global, std::vector<line::extern_ *> externs)
    : global(global) {
    this->externs = externs;
    this->instructions.push_back(
        new line::instruction(instruction::mov, {new operand::reg(reg::ebp),
                                                 new operand::reg(reg::esp)}));
}
void mov_memory(std::vector<line::line *> &instructions, operand::operand *op1,
                operand::operand *op2) {
    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({new operand::reg(reg::ecx), op2})));
    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({op1, new operand::reg(reg::ecx)})));
}
void Asm::add_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper2 =
        operand::operand::get_operand(q.arg2, stack_size, instructions);
    operand::operand *oper3 =
        operand::operand::get_operand(q.result, stack_size, instructions);
    if (typeid(*oper1) == typeid(operand::memory)) {
        mov_memory(instructions, oper3, oper1);
    } else {
        instructions.push_back(new line::instruction(
            instruction::mov, vector<operand::operand *>({oper3, oper1})));
    }
    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({new operand::reg(reg::ecx), oper2})));
    instructions.push_back(new line::instruction(
        instruction::add,
        vector<operand::operand *>({oper3, new operand::reg(reg::ecx)})));
}
void Asm::sub_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper2 =
        operand::operand::get_operand(q.arg2, stack_size, instructions);
    operand::operand *oper3 =
        operand::operand::get_operand(q.result, stack_size, instructions);
    if (typeid(*oper1) == typeid(operand::memory)) {
        mov_memory(instructions, oper3, oper1);
    } else {
        instructions.push_back(new line::instruction(
            instruction::mov, vector<operand::operand *>({oper3, oper1})));
    }
    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({new operand::reg(reg::ecx), oper2})));
    instructions.push_back(new line::instruction(
        instruction::sub,
        vector<operand::operand *>({oper3, new operand::reg(reg::ecx)})));
}
void Asm::mul_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper2;
    if (q.arg2->type == "num") {
        oper2 = new operand::reg(reg::ebx);
        instructions.push_back(new line::instruction(
            instruction::mov, {new operand::reg(reg::ebx),
                               new operand::immediate(q.arg2->value)}));

    } else {
        oper2 = operand::operand::get_operand(q.arg2, stack_size, instructions);
    }
    operand::operand *oper3 =
        operand::operand::get_operand(q.result, stack_size, instructions);
    operand::reg *eax = new operand::reg("eax");
    instructions.push_back(new line::instruction(
        instruction::mov, vector<operand::operand *>({eax, oper1})));
    instructions.push_back(new line::instruction(
        instruction::imul, vector<operand::operand *>({oper2})));
    instructions.push_back(new line::instruction(
        instruction::mov, vector<operand::operand *>({oper3, eax})));
}
void Asm::div_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper2;
    if (q.arg2->type == "num") {
        oper2 = new operand::reg(reg::ebx);
        instructions.push_back(new line::instruction(
            instruction::mov, {new operand::reg(reg::ebx),
                               new operand::immediate(q.arg2->value)}));
    } else {
        oper2 = operand::operand::get_operand(q.arg2, stack_size, instructions);
    }
    operand::operand *oper3 =
        operand::operand::get_operand(q.result, stack_size, instructions);

    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({new operand::reg(reg::eax), oper1})));
    instructions.push_back(new line::instruction(
        instruction::cdq, vector<operand::operand *>({})));
    instructions.push_back(new line::instruction(
        instruction::idiv, vector<operand::operand *>({oper2})));
    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({oper3, new operand::reg(reg::eax)})));
}
void Asm::mod_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper2;
    if (q.arg2->type == "num") {
        oper2 = new operand::reg(reg::ebx);
        instructions.push_back(new line::instruction(
            instruction::mov, {new operand::reg(reg::ebx),
                               new operand::immediate(q.arg2->value)}));
    } else {
        oper2 = operand::operand::get_operand(q.arg2, stack_size, instructions);
    }
    operand::operand *oper3 =
        operand::operand::get_operand(q.result, stack_size, instructions);
    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({new operand::reg(reg::eax), oper1})));
    instructions.push_back(new line::instruction(
        instruction::cdq, vector<operand::operand *>({})));
    instructions.push_back(new line::instruction(
        instruction::idiv, vector<operand::operand *>({oper2})));
    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({oper3, new operand::reg(reg::edx)})));
}
void Asm::j_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper2 =
        operand::operand::get_operand(q.arg2, stack_size, instructions);
    instructions.push_back(new line::instruction(
        instruction::mov,
        vector<operand::operand *>({new operand::reg(reg::ecx), oper2})));
    instructions.push_back(new line::instruction(
        instruction::cmp,
        vector<operand::operand *>({oper1, new operand::reg(reg::ecx)})));
    instructions.push_back(new line::instruction(
        this->get_condition_jmp.at(q.op),
        vector<operand::operand *>(
            {new operand::label(string("l") + string(q.result->value))})));
    this->labels.insert(atoi(q.result->value));
}
void Asm::jmp_handler(quadruple q) {
    instructions.push_back(new line::instruction(
        instruction::jmp, vector<operand::operand *>({new operand::label(
                              string("l") + string(q.result->value))})));
    this->labels.insert(atoi(q.result->value));
}
void Asm::call_handler(quadruple q) {
    instructions.push_back(new line::instruction(
        instruction::call, {new operand::label(string(q.arg1->value))}));
    // 栈顶弹出调用函数所用的参数
    instructions.push_back(new line::instruction(
        instruction::add,
        {new operand::reg(reg::esp),
         new operand::immediate(4 * std::atoi(q.arg2->value))}));
}
void Asm::param_handler(quadruple q) {
    // 然后把字符串对应的var值和其余参数压栈
    operand::operand *oper =
        operand::operand::get_operand(q.result, stack_size, instructions);
    instructions.push_back(new line::instruction(instruction::push, {oper}));
}
void Asm::assign_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper3 =
        operand::operand::get_operand(q.result, stack_size, instructions);
    if (typeid(*oper1) == typeid(operand::memory)) {
        mov_memory(instructions, oper3, oper1);
    } else {
        instructions.push_back(new line::instruction(
            instruction::mov, vector<operand::operand *>({oper3, oper1})));
    }
}

void Asm::string_handler(quadruple q) {
    variables.push_back(new line::variable(
        string("v") + std::to_string(q.result->offset), q.arg1->value));
}

void Asm::address_handler(quadruple q) {
    operand::operand *oper =
        operand::operand::get_operand(q.result, stack_size, instructions);

    instructions.push_back(new line::instruction(
        instruction::mov, {oper, new operand::reg(reg::ebp)}));
    instructions.push_back(new line::instruction(
        instruction::sub, {oper, new operand::immediate(q.arg1->offset)}));
}

void Asm::return_handler(quadruple q) { return; }

void stack_resize(int &stack_size, long long new_size,
                  vector<line::line *> &lines) {
    if (new_size <= stack_size) {
        return;
    }
    lines.push_back(new line::instruction(
        instruction::sub,
        vector<operand::operand *>(
            {new operand::reg(reg::esp),
             new operand::immediate(new_size - stack_size)})));
    stack_size = new_size;
}

void Asm::handle(vector<quadruple *> quadruples) {
    for (int i = 0; i < quadruples.size(); ++i) {
        instructions.push_back(
            new line::label(string("l") + std::to_string(i)));
        (this->*(this->handlers.at(quadruples[i]->op)))(*quadruples[i]);
    }
    int i = 0;
    auto instruction = instructions.begin();
    while (instruction != instructions.end()) {
        if (typeid(**instruction) == typeid(line::label)) {
            if (labels.find(i++) == labels.end()) {
                instruction = instructions.erase(instruction);
                continue;
            }
        }
        instruction++;
    }
}
std::vector<std::string> Asm::to_lines() {
    vector<string> lines;

    for (auto line : externs) {
        lines.push_back(line->to_string());
    }
    lines.push_back(line::section("data").to_string());
    for (auto variable : variables) {
        lines.push_back(variable->to_string());
    }
    lines.push_back(line::section("text").to_string());

    lines.push_back("global " + global.get_var());
    lines.push_back(global.to_string());
    for (auto instruction : instructions) {
        lines.push_back(instruction->to_string());
    }
    lines.push_back("push 0");
    lines.push_back("call exit");
    return lines;
}

Asm::~Asm() {
    for (auto variable : variables) {
        delete variable;
    }
    for (auto instruction : instructions) {
        delete instruction;
    }
    for (auto extern_ : externs) {
        delete extern_;
    }
}

extern "C" void print_lines() {
    extern std::vector<quadruple *> quadruple_array;
    Asm asm_;
    asm_.handle(quadruple_array);
    for (auto line : asm_.to_lines()) {
        cout << line << endl;
    }
}

// int main() {
//     Asm asm_;
//     quadruple *a = new quadruple("assign", new address3("int_value", "1"),
//                                  nullptr, new address3("int", 0, 4));
//     quadruple *b = new quadruple("assign", new address3("int_value", "2"),
//                                  nullptr, new address3("int", 4, 4));
//     quadruple *c = new quadruple(">=", new address3("int", 0, 4),
//                                  new address3("int", 4, 4),
//                                  new address3("int_value", "0"));
//     quadruple *d = new quadruple("<=", new address3("int", 0, 4),
//                                  new address3("int", 4, 4),
//                                  new address3("int_value", "0"));
//     quadruple *e = new quadruple("==", new address3("int", 0, 4),
//                                  new address3("int", 4, 4),
//                                  new address3("int_value", "0"));
//     quadruple *f =
//         new quadruple("<", new address3("int", 0, 4), new address3("int", 4,
//         4),
//                       new address3("int_value", "0"));
//     quadruple *g =
//         new quadruple(">", new address3("int", 0, 4), new address3("int", 4,
//         4),
//                       new address3("int_value", "0"));
//     quadruple *h = new quadruple("!=", new address3("int", 0, 4),
//                                  new address3("int", 4, 4),
//                                  new address3("int_value", "0"));
//     quadruple *i =
//         new quadruple("goto", nullptr, nullptr, new address3("int_value",
//         "2"));
//     quadruple *j =
//         new quadruple("string", new address3("string_value", "%d\\n"),
//         nullptr,
//                       new address3("string", 1, 3));
//     quadruple *k =
//         new quadruple("param", nullptr, nullptr, new address3("string", 1,
//         3));
//     quadruple *l = new quadruple("call", new address3("string_value",
//     "printf"),
//                                  new address3("int_value", "2"), nullptr);
//     asm_.handle(vector<quadruple *>({a, b, c, d, e, f, g, h, i, j, k, l}));
//     for (auto line : asm_.to_lines()) {
//         printf("%s\n", line.c_str());
//     }
// }