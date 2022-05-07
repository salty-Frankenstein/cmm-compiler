#include "ir.h"
#include "parser.h"
#include<stdio.h>
#include<assert.h>

#define GET_OP(instp, i) ((instp)->addrs[i])

CONS(Oprand, makeLabelOp, OP_LABEL, char*, label)
CONS(Oprand, makeVarOp, OP_VAR, char*, name)
CONS(Oprand, makeLitOp, OP_LIT, int, lit)

bool isRVal(Oprand* op) {
    return op->tag == OP_LIT || op->tag == OP_VAR;
}

bool isLVal(Oprand* op) {
    return op->tag == OP_VAR;
}

// safe constructors
Instruction* makeUnaryInst(enum InstKind tag, Oprand* op) {
    switch (tag) {
    case I_LABEL:
    case I_FUNC:
    case I_GOTO:
        assert(op->tag == OP_LABEL);
        break;
    case I_RET:
    case I_ARG:
    case I_PARAM:
    case I_READ:
    case I_WRITE:
        assert(isRVal(op));
        break;
    default:
        assert(0);
    }
    NEW(Instruction, res);
    res->tag = tag;
    res->addrs[0] = op;
    return res;
}

Instruction* makeBinaryInst(enum InstKind tag, Oprand* op1, Oprand* op2) {
    switch (tag) {
    case I_ASSGN:
        assert(isLVal(op1) && isRVal(op2));
        break;
    case I_ADDR:
        assert(isLVal(op1) && isLVal(op2));
        break;
    case I_LOAD:
        assert(isLVal(op1) && isRVal(op2));
        break;
    case I_SAVE:
        assert(isRVal(op1) && isRVal(op2));
        break;
    case I_DEC:
        assert(op1->tag == OP_VAR);
        assert(op2->tag == OP_LIT && op2->content.lit % 4 == 0);
        break;
    case I_CALL:
        assert(isLVal(op1));
        assert(op2->tag == OP_LABEL);
        break;
    default:
        assert(0);
        break;
    }

    NEW(Instruction, res);
    res->tag = tag;
    res->addrs[0] = op1;
    res->addrs[1] = op2;
    return res;
}

Instruction* makeTernaryInst(enum InstKind tag, Oprand* op1, Oprand* op2, Oprand* op3) {
    switch (tag) {
    case I_ADD:
    case I_SUB:
    case I_MUL:
    case I_DIV:
        assert(isLVal(op1) && isRVal(op2) && isRVal(op3));
        break;
    case I_EQGOTO:
    case I_NEGOTO:
    case I_LTGOTO:
    case I_GTGOTO:
    case I_LEGOTO:
    case I_GEGOTO:
        assert(isRVal(op1) && isRVal(op2) && op3->tag == OP_LABEL);
        break;
    default:
        assert(0);
        break;
    }
    NEW(Instruction, res);
    res->tag = tag;
    res->addrs[0] = op1;
    res->addrs[1] = op2;
    res->addrs[2] = op3;
    return res;
}

void printOprand(const Oprand* op) {
    switch (op->tag) {
    case OP_LABEL:
        printf("%s", op->content.label);
        break;
    case OP_LIT:
        printf("#%d", op->content.lit);
        break;
    case OP_VAR:
        printf("%s", op->content.name);
        break;
    default:
        assert(0);
    }
}

void printStrOp1(const Instruction* i, const char* str) {
    printf(str);
    printOprand(GET_OP(i, 0));
}

void printOp1StrOp2(const Instruction* i, const char* str) {
    printOprand(GET_OP(i, 0));
    printf(str);
    printOprand(GET_OP(i, 1));
}

void printArith(const Instruction* i, const char* op) {
    printOp1StrOp2(i, " := ");
    printf(op);
    printOprand(GET_OP(i, 2));
}

void printRelGoto(const Instruction* i, const char* op) {
    printf("IF ");
    printOp1StrOp2(i, op);
    printf(" GOTO ");
    printOprand(GET_OP(i, 2));
}

void printInst(const Instruction* i) {
    switch (i->tag) {
    case I_LABEL: printStrOp1(i, "LABEL "); printf(" :"); break;
    case I_FUNC: printStrOp1(i, "FUNCTION "); printf(" :"); break;
    case I_ASSGN: printOp1StrOp2(i, " := "); break;
    case I_ADD: printArith(i, " + "); break;
    case I_SUB: printArith(i, " - "); break;
    case I_MUL: printArith(i, " * "); break;
    case I_DIV: printArith(i, " / "); break;
    case I_ADDR: printOp1StrOp2(i, " := &"); break;
    case I_LOAD: printOp1StrOp2(i, " := *"); break;
    case I_SAVE: printOp1StrOp2(i, "* := "); break;
    case I_GOTO: printStrOp1(i, "GOTO "); break;
    case I_EQGOTO: printRelGoto(i, " == "); break;
    case I_NEGOTO: printRelGoto(i, " != "); break;
    case I_LTGOTO: printRelGoto(i, " < "); break;
    case I_GTGOTO: printRelGoto(i, " > "); break;
    case I_LEGOTO: printRelGoto(i, " <= "); break;
    case I_GEGOTO: printRelGoto(i, " >= "); break;
    case I_RET: printStrOp1(i, "RETURN "); break;
    case I_DEC:
        printStrOp1(i, "DEC ");
        printf(" %d ", GET_OP(i, 1)->content.lit);
        break;
    case I_ARG: printStrOp1(i, "ARG "); break;
    case I_CALL: printOp1StrOp2(i, " := CALL "); break;
    case I_PARAM: printStrOp1(i, "PARAM "); break;
    case I_READ: printStrOp1(i, "READ "); break;
    case I_WRITE: printStrOp1(i, "WRITE "); break;
    default:assert(0);
    }
}

void printIR(const IR* ir) {
    IRNode* i;
    // skip the first dummy node
    for(i = ir->head->next; i != NULL; i = i->next) {
        printInst(i->inst);
        printf("\n");
    }
}

IRNode* makeIRNode(const Instruction* inst) {
    NEW(IRNode, res);
    res->inst = inst;
    res->next = NULL;
    return res;
}

IR* makeIR() {
    NEW(IR, res);
    NEW(IRNode, dummy);
    res->head = res->tail = dummy;
    return res;
}

// write an instruction to the target ir
void writeInst(IR* target, const Instruction* inst) {
    target->tail->next = makeIRNode(inst);
    target->tail = target->tail->next;
}

IR* testIR() {
    IR* res = makeIR();
    writeInst(res, makeUnaryInst(I_FUNC, makeLabelOp("main")));
    writeInst(res, makeUnaryInst(I_READ, makeVarOp("t1")));
    writeInst(res, makeBinaryInst(I_ASSGN, makeVarOp("v1"), makeVarOp("t1")));
    writeInst(res, makeBinaryInst(I_ASSGN, makeVarOp("t2"), makeLitOp(0)));

    return res;
}