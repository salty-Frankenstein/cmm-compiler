#include "ir.h"
#include "parser.h"
#include "semantics.h"
#include<stdio.h>
#include<assert.h>
#include<string.h>

#define GET_OP(instp, i) ((instp)->addrs[i])

CONS(Oprand, makeLabelOp, OP_LABEL, char*, label)
CONS(Oprand, makeVarOp, OP_VAR, char*, name)
CONS(Oprand, makeLitOp, OP_LIT, int, lit)

enum InstKind getRelOp(enum RelOpTag tag) {
    switch (tag) {
    case LT: return I_LTGOTO;
    case LE: return I_LEGOTO;
    case GT: return I_GTGOTO;
    case GE: return I_GEGOTO;
    case EQ: return I_EQGOTO;
    case NE: return I_NEGOTO;
    default: assert(0);
    }
}

bool isRVal(Oprand* op) {
    return op->tag == OP_LIT || op->tag == OP_VAR;
}

bool isLVal(Oprand* op) {
    return op->tag == OP_VAR;
}

// safe constructors
Instruction* makeUnaryInst(enum InstKind tag, Oprand* op) {
    switch (tag) {
    case I_LABEL: case I_FUNC: case I_GOTO:
        assert(op->tag == OP_LABEL); break;
    case I_RET: case I_ARG: case I_PARAM: case I_READ: case I_WRITE:
        assert(isRVal(op)); break;
    default: assert(0);
    }
    NEW(Instruction, res);
    res->tag = tag;
    res->addrs[0] = op;
    return res;
}

Instruction* makeBinaryInst(enum InstKind tag, Oprand* op1, Oprand* op2) {
    switch (tag) {
    case I_ASSGN: assert(isLVal(op1) && isRVal(op2)); break;
    case I_ADDR: assert(isLVal(op1) && isLVal(op2)); break;
    case I_LOAD: assert(isLVal(op1) && isRVal(op2)); break;
    case I_SAVE: assert(isRVal(op1) && isRVal(op2)); break;
    case I_DEC:
        assert(op1->tag == OP_VAR);
        assert(op2->tag == OP_LIT && op2->content.lit % 4 == 0);
        break;
    case I_CALL:
        assert(isLVal(op1));
        assert(op2->tag == OP_LABEL);
        break;
    default: assert(0);
    }

    NEW(Instruction, res);
    res->tag = tag;
    res->addrs[0] = op1;
    res->addrs[1] = op2;
    return res;
}

Instruction* makeTernaryInst(enum InstKind tag, Oprand* op1, Oprand* op2, Oprand* op3) {
    switch (tag) {
    case I_ADD: case I_SUB: case I_MUL: case I_DIV:
        assert(isLVal(op1) && isRVal(op2) && isRVal(op3)); break;
    case I_EQGOTO: case I_NEGOTO: case I_LTGOTO: case I_GTGOTO: case I_LEGOTO: case I_GEGOTO:
        assert(isRVal(op1) && isRVal(op2) && op3->tag == OP_LABEL); break;
    default: assert(0);
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
    case OP_LABEL: printf("%s", op->content.label); break;
    case OP_LIT: printf("#%d", op->content.lit); break;
    case OP_VAR: printf("%s", op->content.name); break;
    default: assert(0);
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
    for (i = ir->head->next; i != NULL; i = i->next) {
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

/* yield a fresh temp variable */
Oprand* newTempVar() {
    static int no = 0;
    char* res = (char*)malloc(NAME_SIZE);
    sprintf(res, "t$%X", no);
    no++;
    return makeVarOp(res);
}

/* yield a fresh label */
Oprand* newLabel() {
    static int no = 0;
    char* res = (char*)malloc(NAME_SIZE);
    sprintf(res, "label$%X", no);
    no++;
    return makeLabelOp(res);
}

void translateProgram(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == Program);
    translateExtDefList(target, GET_CHILD(root, 0), table);
}

void translateExtDefList(IR* target, Node* root, SymbolTable table) {
    if (root == NULL) { return; }
    assert(root->content.nonterminal.childNum == 2);
    translateExtDef(target, GET_CHILD(root, 0), table);
    translateExtDefList(target, GET_CHILD(root, 1), table);
}

void translateExtDef(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == ExtDef);
    if (PATTERN3(root, Specifier, ExtDecList, _)) { // ExtDef -> Specifier ExtDecList SEMI
        // TODO: what to do?
    }
    else if (PATTERN2(root, Specifier, _)) { // ExtDef -> Specifier SEMI
        // TODO: what to do?
    }
    else if (PATTERN3(root, Specifier, FunDec, CompSt)) { // function definition
        translateFuncParam(target, GET_CHILD(root, 1), table);
        translateCompSt(target, GET_CHILD(root, 2), table);
    }
    else if (PATTERN3(root, Specifier, FunDec, TOKEN)) { // function declaration 
        printf("function declaration is not available");
        assert(0);
    }
}

// quite special one, for function parameter preparation
void translateFuncParam(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == FunDec);
    if (PATTERN4(root, TOKEN, _, VarList, _)) {
        // the signature information can be obtained from the table
        char* fname = GET_TERMINAL(GET_CHILD(root, 0), reprS);
        SymbolTableNode* p;
        FuncSignature* fs = NULL;
        for (p = table; p != NULL; p = p->next) {
            if (p->content->tag == S_FUNC &&
                strcmp(p->content->content.funcDef->name, fname) == 0) {
                assert(p->content->content.funcDef->type->tag == FUNC);
                fs = p->content->content.funcDef->type->content.func;
                break;
            }
        }
        assert(fs != NULL);
        RecordField* q;
        for(q = fs->params; q != NULL; q = q->next) {
            writeInst(target, makeUnaryInst(I_PARAM, makeVarOp(q->name)));
        }
    }
    else if (PATTERN3(root, TOKEN, _, _)) {
        // nothing to do here
    }
}

// generate code to `target` and return a list of temp variables of the args
ArgList* translateArgs(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == Args);
    if (PATTERN(root, Exp)) {
        Oprand* t1 = newTempVar();
        translateExp(target, GET_CHILD(root, 0), table, t1);
        NEW(ArgList, res);
        res->argVal = t1;
        res->next = NULL;
        return res;
    }
    else if (PATTERN3(root, Exp, _, Args)) {
        Oprand* t1 = newTempVar();
        translateExp(target, GET_CHILD(root, 0), table, t1);
        NEW(ArgList, res);
        res->argVal = t1;
        res->next = translateArgs(target, GET_CHILD(root, 2), table);
        return res;
    }
    CATCH_ALL
}

#define DO_TRANSLATE_ARITH(tag) \
    do {\
        assert(tag == I_ADD || tag == I_SUB || tag == I_MUL || tag == I_DIV);   \
        Oprand* t1 = newTempVar();  \
        Oprand* t2 = newTempVar();  \
        translateExp(target, exp1, table, t1);  \
        translateExp(target, exp2, table, t2);  \
        writeInst(target, makeTernaryInst(tag, place, t1, t2));   \
    } while(0);

void translateExp(IR* target, Node* root, SymbolTable table, Oprand* place) {
    assert(root->tag == Exp);
    if (PATTERN3(root, Exp, _, Exp)) {  // binary
        Node* exp1 = GET_CHILD(root, 0);
        Node* exp2 = GET_CHILD(root, 2);
        switch (GET_CHILD(root, 1)->content.terminal->tag) {
        case ASSIGNOP:
            if (PATTERN(exp1, _)) {
                Node* token = GET_CHILD(exp1, 0);
                assert(token->content.terminal->tag == ID);
                char* v = GET_TERMINAL(token, reprS);
                Oprand* t1 = newTempVar();
                translateExp(target, exp2, table, t1);
                writeInst(target, makeBinaryInst(I_ASSGN, makeVarOp(v), t1));
                writeInst(target, makeBinaryInst(I_ASSGN, place, makeVarOp(v)));
                return;
            }
        case PLUS: DO_TRANSLATE_ARITH(I_ADD); break;
        case MINUS: DO_TRANSLATE_ARITH(I_SUB); break;
        case STAR: DO_TRANSLATE_ARITH(I_MUL); break;
        case DIV: DO_TRANSLATE_ARITH(I_DIV); break;
        case RELOP: case AND: case OR: goto cond_expr;
        default: assert(0);
        }
    }
    else if (PATTERN3(root, _, Exp, _)) {   // (Exp)
        translateExp(target, GET_CHILD(root, 1), table, place);
        return;
    }
    else if (PATTERN2(root, _, Exp)) {  // -Exp & !Exp
        switch (GET_CHILD(root, 0)->content.terminal->tag) {
        case MINUS:
        {
            Oprand* t1 = newTempVar();
            translateExp(target, GET_CHILD(root, 1), table, t1);
            writeInst(target, makeTernaryInst(I_SUB, place, makeLitOp(0), t1));
            break;
        }
        case NOT: goto cond_expr;
        default: assert(0);
        }
    }
    else if (PATTERN4(root, _, _, Args, _)) {   // ID(Args)
        char* fname = GET_TERMINAL(GET_CHILD(root, 0), reprS);
        ArgList* args = translateArgs(target, GET_CHILD(root, 2), table);
        if (strcmp(fname, "write") == 0) {
            writeInst(target, makeUnaryInst(I_WRITE, args->argVal));
            writeInst(target, makeBinaryInst(I_ASSGN, place, makeLitOp(0)));
        }
        else {
            for (; args != NULL; args = args->next) {
                writeInst(target, makeUnaryInst(I_ARG, args->argVal));
            }
            writeInst(target, makeBinaryInst(I_CALL, place, makeLabelOp(fname)));
        }
    }
    else if (PATTERN3(root, _, _, _)) {      // ID()
        char* fname = GET_TERMINAL(GET_CHILD(root, 0), reprS);
        if (strcmp(fname, "read") == 0) {
            writeInst(target, makeUnaryInst(I_READ, place));
        }
        else {
            writeInst(target, makeBinaryInst(I_CALL, place, makeLabelOp(fname)));
        }
    }
    else if (PATTERN4(root, Exp, _, Exp, _)) {   // Exp[Exp]
        // TODO
        assert(0);
    }
    else if (PATTERN3(root, Exp, _, _)) {    // Exp.ID
        // TODO
        assert(0);
    }
    else if (PATTERN(root, TOKEN)) {         // lit & id, base case
        Node* token = GET_CHILD(root, 0);
        int i;
        char* v;
        switch (token->content.terminal->tag) {
        case INT:
            i = GET_TERMINAL(token, intLit);
            writeInst(target, makeBinaryInst(I_ASSGN, place, makeLitOp(i)));
            break;
        case ID:
            v = GET_TERMINAL(token, reprS);
            writeInst(target, makeBinaryInst(I_ASSGN, place, makeVarOp(v)));
            break;
        case FLOAT:
            printf("float literal is not available");
            assert(0);
        default: assert(0);
        }
    }
    CATCH_ALL;
    return;

cond_expr:
    {
        Oprand* l1 = newLabel();
        Oprand* l2 = newLabel();
        writeInst(target, makeBinaryInst(I_ASSGN, place, makeLitOp(0)));
        translateCond(target, root, l1, l2, table);
        writeInst(target, makeUnaryInst(I_LABEL, l1));
        writeInst(target, makeBinaryInst(I_ASSGN, place, makeLitOp(1)));
        writeInst(target, makeUnaryInst(I_LABEL, l2));
    }
}

void translateCond(IR* target, Node* root, Oprand* labelTrue, Oprand* labelFalse, SymbolTable table) {
    assert(root->tag == Exp);
    assert(labelTrue->tag == OP_LABEL);
    assert(labelFalse->tag == OP_LABEL);
    if (PATTERN3(root, Exp, _, Exp)) {
        Node* exp1 = GET_CHILD(root, 0);
        Node* op = GET_CHILD(root, 1);
        Node* exp2 = GET_CHILD(root, 2);
        switch (op->content.terminal->tag) {
        case RELOP:
        {
            Oprand* t1 = newTempVar();
            Oprand* t2 = newTempVar();
            translateExp(target, exp1, table, t1);
            translateExp(target, exp2, table, t2);
            enum InstKind ik = getRelOp(GET_TERMINAL(op, relOp));
            writeInst(target, makeTernaryInst(ik, t1, t2, labelTrue));
            writeInst(target, makeUnaryInst(I_GOTO, labelFalse));
            return;
        }
        case AND:
        {
            Oprand* l1 = newLabel();
            translateCond(target, exp1, l1, labelFalse, table);
            writeInst(target, makeUnaryInst(I_LABEL, l1));
            translateCond(target, exp2, labelTrue, labelFalse, table);
            return;
        }
        case OR:
        {
            Oprand* l1 = newLabel();
            translateCond(target, exp1, labelTrue, l1, table);
            writeInst(target, makeUnaryInst(I_LABEL, l1));
            translateCond(target, exp2, labelTrue, labelFalse, table);
            return;
        }
        default: goto otherwise;
        }
    }
    else if (PATTERN2(root, _, Exp)) {
        switch (GET_CHILD(root, 0)->content.terminal->tag) {
        case NOT:
            translateCond(target, GET_CHILD(root, 1), labelFalse, labelTrue, table);
            return;
        default: goto otherwise;
        }
    }
    else {
        goto otherwise;
    }
    return;

otherwise:
    {
        Oprand* t1 = newTempVar();
        translateExp(target, root, table, t1);
        writeInst(target, makeTernaryInst(I_NEGOTO, t1, makeLitOp(0), labelTrue));
        writeInst(target, makeUnaryInst(I_GOTO, labelFalse));
    }
}

void translateStmt(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == Stmt);
    if (PATTERN2(root, Exp, _)) {   // Exp;
        Oprand* t = newTempVar();
        translateExp(target, GET_CHILD(root, 0), table, t);
        // TODO: the temp var can be eliminated
        return;
    }
    else if (PATTERN(root, CompSt)) {
        // TODO
        assert(0);
    }
    else if (PATTERN3(root, _, Exp, _)) {    // return Exp;
        Oprand* t1 = newTempVar();
        translateExp(target, GET_CHILD(root, 1), table, t1);
        writeInst(target, makeUnaryInst(I_RET, t1));
        return;
    }
    else if (PATTERN5(root, _, _, Exp, _, Stmt)) {  // if(Exp) Stmt || while(Exp) Stmt
        switch (GET_CHILD(root, 0)->content.terminal->tag) {
        case IF:
        {
            Oprand* l1 = newLabel();
            Oprand* l2 = newLabel();
            translateCond(target, GET_CHILD(root, 2), l1, l2, table);
            writeInst(target, makeUnaryInst(I_LABEL, l1));
            translateStmt(target, GET_CHILD(root, 4), table);
            writeInst(target, makeUnaryInst(I_LABEL, l2));
            return;
        }
        case WHILE:
        {
            Oprand* l1 = newLabel();
            Oprand* l2 = newLabel();
            Oprand* l3 = newLabel();
            writeInst(target, makeUnaryInst(I_LABEL, l1));
            translateCond(target, GET_CHILD(root, 2), l2, l3, table);
            writeInst(target, makeUnaryInst(I_LABEL, l2));
            translateStmt(target, GET_CHILD(root, 4), table);
            writeInst(target, makeUnaryInst(I_GOTO, l1));
            writeInst(target, makeUnaryInst(I_LABEL, l3));
            return;
        }
        default: assert(0);
        }
    }
    else if (PATTERN7(root, _, _, Exp, _, Stmt, _, Stmt)) {
        Oprand* l1 = newLabel();
        Oprand* l2 = newLabel();
        Oprand* l3 = newLabel();
        translateCond(target, GET_CHILD(root, 2), l1, l2, table);
        writeInst(target, makeUnaryInst(I_LABEL, l1));
        translateStmt(target, GET_CHILD(root, 4), table);
        writeInst(target, makeUnaryInst(I_GOTO, l3));
        writeInst(target, makeUnaryInst(I_LABEL, l2));
        translateStmt(target, GET_CHILD(root, 6), table);
        writeInst(target, makeUnaryInst(I_LABEL, l3));
        return;
    }
    CATCH_ALL
}

void translateCompSt(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == CompSt);
    if (root->content.nonterminal.childNum == 4) {  // { DefList StmtList }
        // TODO: what to do with the definitions?
        translateStmtList(target, GET_CHILD(root, 2), table);
    }
    CATCH_ALL
}

void translateStmtList(IR* target, Node* root, SymbolTable table) {
    if (root == NULL) { return; }
    assert(root->tag == StmtList);
    translateStmt(target, GET_CHILD(root, 0), table);
    translateStmtList(target, GET_CHILD(root, 1), table);
}
