#include "asm_generator.h"
using namespace std;

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
    return this->type + " [ebp-" + std::to_string(this->offset) + "]";
}
string operand::reg::to_string() { return this->name; }

extern void stack_resize(int &stack_size, int offset,
                         std::vector<line::line *> &lines);

operand::operand *
operand::operand::get_operand(address3 *addr, int &stack_size,
                              std::vector<line::line *> &lines) {
    if (addr->type == "num") {
        return new immediate(addr->value);
    }
    stack_resize(stack_size, addr->value, lines);
    return new memory(addr->value);
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
    this->value = this->value.substr(1, this->value.length() - 2);
    return this->name + " db `" + this->value + "`,0";
}
string line::extern_::to_string() { return "extern " + this->function; }
string line::section::to_string() { return "section ." + this->name; }

string line::label::get_var() { return this->var; }

Asm::Asm(line::label global, std::vector<line::extern_ *> externs)
    : global(global) {
    this->externs = externs;
}

void Asm::add_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper2 =
        operand::operand::get_operand(q.arg2, stack_size, instructions);
    operand::operand *oper3 =
        operand::operand::get_operand(q.result, stack_size, instructions);
    instructions.push_back(new line::instruction(
        instruction::mov, vector<operand::operand *>({oper3, oper1})));
    instructions.push_back(new line::instruction(
        instruction::add, vector<operand::operand *>({oper3, oper2})));
}
void Asm::sub_handler(quadruple q) {
    operand::operand *oper1 =
        operand::operand::get_operand(q.arg1, stack_size, instructions);
    operand::operand *oper2 =
        operand::operand::get_operand(q.arg2, stack_size, instructions);
    operand::operand *oper3 =
        operand::operand::get_operand(q.result, stack_size, instructions);
    instructions.push_back(new line::instruction(
        instruction::mov, vector<operand::operand *>({oper3, oper1})));
    instructions.push_back(new line::instruction(
        instruction::sub, vector<operand::operand *>({oper3, oper2})));
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
void Asm::je_handler(quadruple q) {}
void Asm::jne_handler(quadruple q) {}
void Asm::ja_handler(quadruple q) {}
void Asm::jae_handler(quadruple q) {}
void Asm::jb_handler(quadruple q) {}
void Asm::jbe_handler(quadruple q) {}
void Asm::jmp_handler(quadruple q) {}
void Asm::call_handler(quadruple q) {}
void Asm::param_handler(quadruple q) {}

void stack_resize(int &stack_size, int offset, vector<line::line *> &lines) {
    if (offset <= stack_size) {
        return;
    }
    lines.push_back(new line::instruction(
        instruction::sub, vector<operand::operand *>(
                              {new operand::reg(reg::esp),
                               new operand::immediate(offset - stack_size)})));
    stack_size = offset;
}

void Asm::handle(vector<quadruple> quadruples) {
    for (int i = 0; i < quadruples.size(); ++i) {
        (this->*(this->handlers.at(quadruples[i].op)))(quadruples[i]);
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

int main() {
    Asm asm_;
    quadruple a("+", new address3("num", 1), new address3("num", 2),
                new address3("add", 0));
    quadruple b("-", new address3("num", 1), new address3("num", 2),
                new address3("add", 4));
    quadruple c("*", new address3("num", 1), new address3("num", 2),
                new address3("add", 8));
    quadruple d("/", new address3("num", 1), new address3("num", 2),
                new address3("add", 12));
    quadruple e("%", new address3("num", 1), new address3("num", 2),
                new address3("add", 16));
    asm_.handle(vector<quadruple>({a, b, c, d, e}));
    for (auto line : asm_.to_lines()) {
        cout << line << endl;
    }
}