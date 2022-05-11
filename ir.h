#ifndef IR_H
#define IR_H
#include"parser.h"
#include"semantics.h"
#include<stdio.h>

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

struct ArgList {
    Oprand* argVal;
    struct ArgList* next;
};
typedef struct ArgList ArgList;

int getElemSize(Type* arrayT);

void translateProgram(IR* target, Node* root, SymbolTable table);
void translateExtDefList(IR* target, Node* root, SymbolTable table);
void translateExtDef(IR* target, Node* root, SymbolTable table);
void translateFuncParam(IR* target, Node* root, SymbolTable table);
ArgList* translateArgs(IR* target, Node* root, SymbolTable table);
void translateExp(IR* target, Node* root, SymbolTable table, Oprand* place);
void translateCond(IR* target, Node* root, Oprand* labelTrue, Oprand* labelFalse, SymbolTable table);
void translateStmt(IR* target, Node* root, SymbolTable table);
void translateCompSt(IR* target, Node* root, SymbolTable table);
void translateStmtList(IR* target, Node* root, SymbolTable table);

void printIR(FILE* out, const IR* ir);
IR* makeIR();
IR* testIR();

#endif