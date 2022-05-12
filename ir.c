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

void printOprand(FILE* out, const Oprand* op) {
    switch (op->tag) {
    case OP_LABEL: fprintf(out, "%s", op->content.label); break;
    case OP_LIT: fprintf(out, "#%d", op->content.lit); break;
    case OP_VAR: fprintf(out, "%s", op->content.name); break;
    default: assert(0);
    }
}

void printStrOp1(FILE* out, const Instruction* i, const char* str) {
    fprintf(out, str);
    printOprand(out, GET_OP(i, 0));
}

void printOp1StrOp2(FILE* out, const Instruction* i, const char* str) {
    printOprand(out, GET_OP(i, 0));
    fprintf(out, str);
    printOprand(out, GET_OP(i, 1));
}

void printArith(FILE* out, const Instruction* i, const char* op) {
    printOp1StrOp2(out, i, " := ");
    fprintf(out, op);
    printOprand(out, GET_OP(i, 2));
}

void printRelGoto(FILE* out, const Instruction* i, const char* op) {
    fprintf(out, "IF ");
    printOp1StrOp2(out, i, op);
    fprintf(out, " GOTO ");
    printOprand(out, GET_OP(i, 2));
}

void printInst(FILE* out, const Instruction* i) {
    switch (i->tag) {
    case I_LABEL: printStrOp1(out, i, "LABEL "); fprintf(out, " :"); break;
    case I_FUNC: printStrOp1(out, i, "FUNCTION "); fprintf(out, " :"); break;
    case I_ASSGN: printOp1StrOp2(out, i, " := "); break;
    case I_ADD: printArith(out, i, " + "); break;
    case I_SUB: printArith(out, i, " - "); break;
    case I_MUL: printArith(out, i, " * "); break;
    case I_DIV: printArith(out, i, " / "); break;
    case I_ADDR: printOp1StrOp2(out, i, " := &"); break;
    case I_LOAD: printOp1StrOp2(out, i, " := *"); break;
    case I_SAVE: fprintf(out, "*"); printOp1StrOp2(out, i, " := "); break;
    case I_GOTO: printStrOp1(out, i, "GOTO "); break;
    case I_EQGOTO: printRelGoto(out, i, " == "); break;
    case I_NEGOTO: printRelGoto(out, i, " != "); break;
    case I_LTGOTO: printRelGoto(out, i, " < "); break;
    case I_GTGOTO: printRelGoto(out, i, " > "); break;
    case I_LEGOTO: printRelGoto(out, i, " <= "); break;
    case I_GEGOTO: printRelGoto(out, i, " >= "); break;
    case I_RET: printStrOp1(out, i, "RETURN "); break;
    case I_DEC:
        printStrOp1(out, i, "DEC ");
        fprintf(out, " %d ", GET_OP(i, 1)->content.lit);
        break;
    case I_ARG: printStrOp1(out, i, "ARG "); break;
    case I_CALL: printOp1StrOp2(out, i, " := CALL "); break;
    case I_PARAM: printStrOp1(out, i, "PARAM "); break;
    case I_READ: printStrOp1(out, i, "READ "); break;
    case I_WRITE: printStrOp1(out, i, "WRITE "); break;
    default:assert(0);
    }
}

void printIR(FILE* out, const IR* ir) {
    IRNode* i;
    // skip the first dummy node
    for (i = ir->head->next; i != NULL; i = i->next) {
        printInst(out, i->inst);
        fprintf(out, "\n");
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

void insertInst(IR* target, IRNode* node, const Instruction* inst) {
    IRNode* p = node->next;
    node->next = makeIRNode(inst);
    node->next->next = p;
    if (target->tail == node) {
        target->tail = node->next;
    }
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
        // no global variables, as guaranteed
        assert(0);
    }
    else if (PATTERN2(root, Specifier, _)) { // ExtDef -> Specifier SEMI
        // no global variables, as guaranteed
        assert(0);
    }
    else if (PATTERN3(root, Specifier, FunDec, CompSt)) { // function definition
        Node* funDec = GET_CHILD(root, 1);
        char* fname = GET_TERMINAL(GET_CHILD(funDec, 0), reprS);
        writeInst(target, makeUnaryInst(I_FUNC, makeLabelOp(fname)));
        translateFuncParam(target, funDec, table);
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
        for (q = fs->params; q != NULL; q = q->next) {
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
        DO_TRANSLATE_EXP(target, GET_CHILD(root, 0), table, t1);
        NEW(ArgList, res);
        res->argVal = t1;
        res->next = NULL;
        return res;
    }
    else if (PATTERN3(root, Exp, _, Args)) {
        DO_TRANSLATE_EXP(target, GET_CHILD(root, 0), table, t1);
        NEW(ArgList, res);
        res->argVal = t1;
        res->next = translateArgs(target, GET_CHILD(root, 2), table);
        return res;
    }
    CATCH_ALL
}

// a helper function calculating the element size of an array type
int getElemSize(Type* arrayT) {
    assert(arrayT->tag == ARRAY);
    switch (arrayT->content.array->type->tag) {
    case PRIMITIVE: return 4;
    case ARRAY:
    {
        Type* elem = arrayT->content.array->type;
        return getElemSize(elem) * elem->content.array->size;
    }
    case FUNC: case RECORD: assert(0);     // not supported yet
    default: assert(0);
    }
}

// quite special one, only works for `Exp -> ID | Exp[Exp]`
// generates code that calculates the **address** of the array exp
// returns the remainder dimensions' type of the for the current continuation
Type* translateArray(IR* target, Node* root, SymbolTable table, Oprand* place) {
    assert(root->tag == Exp);
    // ID, the base case of the array expression structure
    if (PATTERN(root, TOKEN)
        && GET_CHILD(root, 0)->content.terminal->tag == ID) {
        char* varName = GET_TERMINAL(GET_CHILD(root, 0), reprS);
        // get the array base address
        writeInst(target, makeBinaryInst(I_ADDR, place, makeVarOp(varName)));
        // lookup the table and return the type of the whole array
        SymbolTableNode* p;
        for (p = table; p != NULL; p = p->next) {
            if (p->content->tag == S_VAR &&
                strcmp(p->content->content.varDef->name, varName) == 0) {
                return p->content->content.varDef->type;
            }
        }
        assert(0);
    }
    // recursive case, calculate the current dimension
    else if (PATTERN4(root, Exp, _, Exp, _)) {  // Exp[Exp]
        Oprand* t1 = newTempVar();
        // first calculate the address of the previous dimensions
        // save to $t1
        Type* arrayType = translateArray(target, GET_CHILD(root, 0), table, t1);
        // get the element size of the current dimension, just a static value
        int size = getElemSize(arrayType);
        // calculate the index, save to $t2
        DO_TRANSLATE_EXP(target, GET_CHILD(root, 2), table, t2);
        Oprand* t3 = newTempVar();
        // multiply by the element size
        writeInst(target, makeTernaryInst(I_MUL, t3, t2, makeLitOp(size)));
        // then add on the offset of the current dimension
        writeInst(target, makeTernaryInst(I_ADD, place, t1, t3));
        return arrayType->content.array->type;  // return the remainder type
    }
    else {
        // there is no other way to form an array expression (?)
        assert(0);
    }
}

#define DO_TRANSLATE_ARITH(tag) \
    do {\
        assert(tag == I_ADD || tag == I_SUB || tag == I_MUL || tag == I_DIV);   \
        DO_TRANSLATE_EXP(target, exp1, table, t1);  \
        DO_TRANSLATE_EXP(target, exp2, table, t2);  \
        writeInst(target, makeTernaryInst(tag, place, t1, t2));   \
    } while(0);


// optimization: for lit & id, just return the corresponding oprand, which saves one instruction
// for the other cases, save the result to `place`, and return `NULL`
Oprand* translateExp(IR* target, Node* root, SymbolTable table, Oprand* place) {
    assert(root->tag == Exp);
    if (PATTERN3(root, Exp, _, Exp)) {  // binary
        Node* exp1 = GET_CHILD(root, 0);
        Node* exp2 = GET_CHILD(root, 2);
        switch (GET_CHILD(root, 1)->content.terminal->tag) {
        case ASSIGNOP:
            // variable case
            if (PATTERN(exp1, _)) {
                Node* token = GET_CHILD(exp1, 0);
                assert(token->content.terminal->tag == ID);
                char* v = GET_TERMINAL(token, reprS);
                DO_TRANSLATE_EXP(target, exp2, table, t1);
                writeInst(target, makeBinaryInst(I_ASSGN, makeVarOp(v), t1));
                writeInst(target, makeBinaryInst(I_ASSGN, place, makeVarOp(v)));
            }
            // array case
            else if (PATTERN4(exp1, Exp, _, Exp, _)) {
                // first calculate the address
                Oprand* addr = newTempVar();
                translateArray(target, exp1, table, addr);
                // then calculate the rhs
                DO_TRANSLATE_EXP(target, exp2, table, rhs);
                // then save
                writeInst(target, makeBinaryInst(I_SAVE, addr, rhs));
                // the expression value
                writeInst(target, makeBinaryInst(I_ASSGN, place, rhs));
            }
            break;
        case PLUS: DO_TRANSLATE_ARITH(I_ADD); break;
        case MINUS: DO_TRANSLATE_ARITH(I_SUB); break;
        case STAR: DO_TRANSLATE_ARITH(I_MUL); break;
        case DIV: DO_TRANSLATE_ARITH(I_DIV); break;
        case RELOP: case AND: case OR: goto cond_expr;
        default: assert(0);
        }
    }
    else if (PATTERN3(root, _, Exp, _)) {   // (Exp)
        return translateExp(target, GET_CHILD(root, 1), table, place);
    }
    else if (PATTERN2(root, _, Exp)) {  // -Exp & !Exp
        switch (GET_CHILD(root, 0)->content.terminal->tag) {
        case MINUS:
        {
            DO_TRANSLATE_EXP(target, GET_CHILD(root, 1), table, t1);
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
            IRNode* p = target->tail;
            // in a reversed order
            for (; args != NULL; args = args->next) {
                insertInst(target, p, makeUnaryInst(I_ARG, args->argVal));
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
        // right value here, the left-value case is handled in assign expr
        // first calculate the address
        Oprand* addr = newTempVar();
        translateArray(target, root, table, addr);
        // then dereference
        writeInst(target, makeBinaryInst(I_LOAD, place, addr));
    }
    else if (PATTERN3(root, Exp, _, _)) {    // Exp.ID
        printf("record field is not available");
        assert(0);
    }
    else if (PATTERN(root, TOKEN)) {         // lit & id, base case
        Node* token = GET_CHILD(root, 0);
        int i;
        char* v;
        switch (token->content.terminal->tag) {
        case INT:
            i = GET_TERMINAL(token, intLit);
            return makeLitOp(i);
        case ID:
            v = GET_TERMINAL(token, reprS);
            return makeVarOp(v);
        case FLOAT:
            printf("float literal is not available");
            assert(0);
        default: assert(0);
        }
    }
    CATCH_ALL;
    return NULL;

cond_expr:
    {
        Oprand* l1 = newLabel();
        Oprand* l2 = newLabel();
        writeInst(target, makeBinaryInst(I_ASSGN, place, makeLitOp(0)));
        translateCond(target, root, l1, l2, table);
        writeInst(target, makeUnaryInst(I_LABEL, l1));
        writeInst(target, makeBinaryInst(I_ASSGN, place, makeLitOp(1)));
        writeInst(target, makeUnaryInst(I_LABEL, l2));
        return NULL;
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
            DO_TRANSLATE_EXP(target, exp1, table, t1);
            DO_TRANSLATE_EXP(target, exp2, table, t2);
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
        DO_TRANSLATE_EXP(target, root, table, t1);
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
        translateCompSt(target, GET_CHILD(root, 0), table);
        return;
    }
    else if (PATTERN3(root, _, Exp, _)) {    // return Exp;
        DO_TRANSLATE_EXP(target, GET_CHILD(root, 1), table, t1);
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

// just lookup the symbol table, and return the corresponding definition
NameTypePair* getVarEntry(Node* root, SymbolTable table) {
    assert(root->tag == VarDec);
    if (PATTERN(root, TOKEN)) {
        char* name = GET_TERMINAL(GET_CHILD(root, 0), reprS);
        SymbolTableNode* p;
        for (p = table; p != NULL; p = p->next) {
            if (p->content->tag == S_VAR
                && strcmp(p->content->content.varDef->name, name) == 0) {
                return p->content->content.varDef;
            }
        }
        assert(0);
    }
    else if (PATTERN4(root, VarDec, _, TOKEN, _)) { // VarDec [ Int ]
        return getVarEntry(GET_CHILD(root, 0), table);
    }
    CATCH_ALL
}

void translateDec(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == Dec);
    if (PATTERN(root, VarDec)) {
        // check array here
        NameTypePair* e = getVarEntry(GET_CHILD(root, 0), table);
        if (e->type->tag == ARRAY) {
            char* name = e->name;
            int size = getElemSize(e->type) * e->type->content.array->size;
            writeInst(target, makeBinaryInst(I_DEC, makeVarOp(name), makeLitOp(size)));
        }
    }
    else if (PATTERN3(root, VarDec, _, Exp)) {
        // initialize here
        NameTypePair* e = getVarEntry(GET_CHILD(root, 0), table);
        char* name = e->name;
        DO_TRANSLATE_EXP(target, GET_CHILD(root, 2), table, rhs);
        writeInst(target, makeBinaryInst(I_ASSGN, makeVarOp(name), rhs));
    }
    CATCH_ALL
}

void translateDecList(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == DecList);
    if (PATTERN(root, Dec)) {
        translateDec(target, GET_CHILD(root, 0), table);
    }
    else if (PATTERN3(root, Dec, _, DecList)) {
        translateDec(target, GET_CHILD(root, 0), table);
        translateDecList(target, GET_CHILD(root, 2), table);
    }
    CATCH_ALL
}

void translateDef(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == Def);
    if (PATTERN3(root, Specifier, DecList, _)) {
        translateDecList(target, GET_CHILD(root, 1), table);
    }
    CATCH_ALL
}

void translateDefList(IR* target, Node* root, SymbolTable table) {
    if (root == NULL) return;
    assert(root->tag == DefList);
    if (root->content.nonterminal.childNum == 2) {
        translateDef(target, GET_CHILD(root, 0), table);
        translateDefList(target, GET_CHILD(root, 1), table);
    }
    CATCH_ALL
}

void translateCompSt(IR* target, Node* root, SymbolTable table) {
    assert(root->tag == CompSt);
    if (root->content.nonterminal.childNum == 4) {  // { DefList StmtList }
        translateDefList(target, GET_CHILD(root, 1), table);
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
