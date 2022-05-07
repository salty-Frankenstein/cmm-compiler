#ifndef IR_H
#define IR_H

enum OprandKind { OP_VAR, OP_LIT, OP_LABEL };
typedef struct Oprand {
    enum OprandKind tag;
    union {
        char* name;     // for OP_VAR
        int lit;        // for OP_LIT
        char* label;    // for OP_LABEL
    } content;
} Oprand;

enum InstKind {
    I_LABEL, I_FUNC, I_ASSGN, I_ADD,
    I_SUB, I_MUL, I_DIV, I_ADDR,
    I_LOAD, I_SAVE, I_GOTO, 
    I_EQGOTO, I_NEGOTO, I_LTGOTO, I_GTGOTO, I_LEGOTO, I_GEGOTO,
    I_RET, I_DEC, I_ARG, I_CALL,
    I_PARAM, I_READ, I_WRITE
};
typedef struct Instruction {
    enum InstKind tag;
    Oprand* addrs[3];    // 3 addresses
} Instruction;

struct IRNode {
    Instruction* inst;
    struct IRNode* next;
};
typedef struct IRNode IRNode;

typedef struct IR {
    struct IRNode* head;    // the head is a dummy node
    struct IRNode* tail;    // the last node of the list
} IR;

void printIR(const IR* ir);
IR* testIR();

#endif