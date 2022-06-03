#include "codegen.h"
#include<assert.h>
#include<string.h>

const char* t1 = "$t1";
const char* t2 = "$t2";

void printInst(FILE* out, const Instruction* i);

// print the offset table for debugging
void printOffsetTable(const NameOffsetPair* table, int size) {
    printf("\ttable size %d\n", size);
    int i;
    for (i = 0; i < size; i++) {
        printf("\tvar: %s, offset: %d\n", table[i].name, table[i].offset);
    }
}

/*
    oldfp-> +-----------+
            | ret addr  |
            +-----------+
            | auto vars |
            +-----------+
            | ...       |
            +-----------+
            | args      |
            +-----------+
            | old fp    |
    $fp->   +-----------+
            | ret addr  |
    $fp-4-> +-----------+
            | auto vars |
    $sp->   +-----------+
            | ...       |
caller:
    1. push args
    2. push $fp
    3. $fp <- $sp
    4. push $ra
    5. jump
callee:
    6. push auto vars 
return:
    7. $sp <- $fp
    8. $ra <- 0($fp)
    9. $fp <- 4($fp)
    10.$sp <- $sp + 8 + 4 * arg num
    11.jump
*/
// a helper function, to get the variable info from a defininition
// the result's `name` field for the variable name, and `offset` for its size
// when the input is not a definition, it returns { name = NULL, offset = 0 }
NameOffsetPair getDefVar(const Instruction* i) {
    NameOffsetPair res;
    switch (i->tag) {
    case I_ASSGN:
    case I_ADD:
    case I_SUB:
    case I_MUL:
    case I_DIV:
    case I_ADDR:
    case I_LOAD:
    case I_CALL:
    case I_READ:
        res.name = i->addrs[0]->content.name;
        res.offset = -4;
        res.isparam = false;
        break;
    case I_DEC:
        // according to the semantics of DEC v, v is the first var of the array
        // but in this compiler, this v is never used expect for loading it's address
        // thus, we just assign the array size to the v's size, which reserves the array's memory
        // and v occupies no memory, which is reasonable
        res.name = i->addrs[0]->content.name;
        res.offset = -i->addrs[1]->content.lit;
        res.isparam = false;
        break;
    case I_PARAM:
        // all the arguments will be in the stack for simplicity
        res.name = i->addrs[0]->content.name;
        res.offset = 4;
        res.isparam = true;
        break;
    default:
        // TODO: check other instructions
        res.name = NULL;
        res.offset = 0;
        res.isparam = false;
        break;
    }
    return res;
}

// return a variable offset table of a given function
// it's designed as an array on the heap, for easy allocation & free
// and the size of the returned table is on the parameter
// not so elegant, but is the best solution for C :-(
NameOffsetPair* makeFuncVarTable(const IRNode* begin, const IRNode* end, int* out_size) {
    int len = 0;
    const IRNode* p;
    // the length of the function, an upper bound of var num
    for (p = begin; p != end; p = p->next) {
        len++;
    }

    NameOffsetPair* table;
    *out_size = 0;
    table = (NameOffsetPair*)malloc(len * sizeof(NameOffsetPair));

    bool firstVar = true;
    int lastOffset;
    for (p = begin; p != end; p = p->next) {
        NameOffsetPair var_info = getDefVar(p->inst);
        if (var_info.name != NULL) {
            int i;
            bool in_table = false;
            for (i = 0; i < *out_size; i++) {
                if (strcmp(table[i].name, var_info.name) == 0) {
                    in_table = true;
                    break;
                }
            }

            // push a new variable into the table 
            if (!in_table) {
                table[*out_size].name = var_info.name;
                // HACK: note that the PARAM declarations should always at the front
                // the offset field is a prefix sum
                // params' offsets are positive, and variables' are negative
                if (var_info.isparam) {
                    if (*out_size == 0) {
                        table[*out_size].offset = var_info.offset;
                    }
                    else {
                        table[*out_size].offset = var_info.offset
                            + table[*out_size - 1].offset;
                    }
                }
                else {
                    if (firstVar) {
                        // set the start address of the variables
                        table[*out_size].offset = -4;
                        firstVar = false;
                    }
                    else {
                        table[*out_size].offset = lastOffset
                            + table[*out_size - 1].offset;
                    }
                    lastOffset = var_info.offset;
                }
                (*out_size)++;
            }
        }
    }

    // then all the variables are collected, the no replicated
    return table;
}

// a helper function to get the code fragment of a given function
// return the **next** instruction of the function
// for the last function, it is `NULL`
IRNode* getFunctionEnd(const IRNode* begin) {
    assert(begin->inst->tag == I_FUNC);
    IRNode* p = begin->next;
    for (; p != NULL && p->inst->tag != I_FUNC; p = p->next);
    return p;
}

NameOffsetPair getOffsetEntry(const NameOffsetPair* table, int tsize, const char* name) {
    int i;
    for (i = 0; i < tsize; i++) {
        if (strcmp(table[i].name, name) == 0) {
            return table[i];
        }
    }
    assert(0);
}

// generate code for var op & lit which needs to be loaded to a reg
// and return the reg (string)
// since instructions have at most 2 oprands
// `pos` decides which reg to choose
void oprandLoad(FILE* out, NameOffsetPair* table, int tsize, Oprand* op, const char* reg) {
    assert(op->tag != OP_LABEL);
    if (op->tag == OP_LIT) {
        fprintf(out, "\tli %s, %d\n", reg, op->content.lit);
    }
    else {
        NameOffsetPair e = getOffsetEntry(table, tsize, op->content.name);
        // then load offset($fp) to the destination
        fprintf(out, "\tlw %s, %d($fp)\n", reg, e.offset);
    }
}

void oprandSave(FILE* out, NameOffsetPair* table, int tsize, Oprand* op, const char* reg) {
    assert(op->tag == OP_VAR);
    NameOffsetPair e = getOffsetEntry(table, tsize, op->content.name);
    fprintf(out, "\tsw %s, %d($fp)\n", reg, e.offset);
}

void generateGoto(FILE* out, NameOffsetPair* table, int t_size, const Instruction* i, char* inst) {
    oprandLoad(out, table, t_size, i->addrs[0], t1);
    oprandLoad(out, table, t_size, i->addrs[1], t2);
    fprintf(out, "\t%s %s, %s, %s\n", inst, t1, t2, i->addrs[2]->content.label);
}
// this function is designed to be called in the instruction order only
// calls out of order would lead to unexpected behaviors
void generateInst(FILE* out, const IRNode* irn) {
    // the table is managed by this function itself
    // it updates the table when it enters a new function,
    // and delete the old one
    // for a pseudo one-pass implementation
    static NameOffsetPair* table = NULL;
    static int tableSize;
    const Instruction* i = irn->inst;

    switch (i->tag) {
    case I_LABEL:
        fprintf(out, "%s:\n", i->addrs[0]->content.label);
        break;
    case I_FUNC:
        // first clean the old table
        if (table != NULL) { free(table); }
        table = makeFuncVarTable(irn, getFunctionEnd(irn), &tableSize);
        // TODO: init func here
        fprintf(out, "\n%s:\n", i->addrs[0]->content.label);
        printOffsetTable(table, tableSize);
        break;
    case I_ASSGN:
        if (i->addrs[1]->tag == OP_LIT) {
            // just save to the addr
            NameOffsetPair e = getOffsetEntry(table, tableSize, i->addrs[0]->content.name);
            fprintf(out, "\tsw #%d, %d($fp)\n", i->addrs[1]->content.lit, e.offset);
        }
        else {
            assert(i->addrs[1]->tag == OP_VAR);
            oprandLoad(out, table, tableSize, i->addrs[1], t1);
            oprandSave(out, table, tableSize, i->addrs[0], t1);
        }
        break;
    case I_ADD:
        // since constant folding is performed, then there would be at most 1 lit-op
        if (i->addrs[2]->tag == OP_LIT) {
            oprandLoad(out, table, tableSize, i->addrs[1], t1);
            fprintf(out, "\taddi %s, %s, %d\n", t1, t1, i->addrs[2]->content.lit);
            oprandSave(out, table, tableSize, i->addrs[0], t1);
        }
        else {
            if (i->addrs[1]->tag == OP_LIT) {
                oprandLoad(out, table, tableSize, i->addrs[2], t1);
                fprintf(out, "\taddi %s, %s, %d\n", t1, t1, i->addrs[1]->content.lit);
                oprandSave(out, table, tableSize, i->addrs[0], t1);
            }
            else {
                oprandLoad(out, table, tableSize, i->addrs[1], t1);
                oprandLoad(out, table, tableSize, i->addrs[2], t2);
                fprintf(out, "\tadd %s, %s, %s\n", t1, t1, t2);
                oprandSave(out, table, tableSize, i->addrs[0], t1);
            }
        }
        break;
    case I_SUB:
        if (i->addrs[2]->tag == OP_LIT) {
            oprandLoad(out, table, tableSize, i->addrs[1], t1);
            fprintf(out, "\taddi %s, %s, %d\n", t1, t1, -i->addrs[2]->content.lit);
            oprandSave(out, table, tableSize, i->addrs[0], t1);
        }
        else {
            oprandLoad(out, table, tableSize, i->addrs[1], t1);
            oprandLoad(out, table, tableSize, i->addrs[2], t2);
            fprintf(out, "\tsub %s, %s, %s\n", t1, t1, t2);
            oprandSave(out, table, tableSize, i->addrs[0], t1);
        }
        break;
    case I_MUL:
    {
        oprandLoad(out, table, tableSize, i->addrs[1], t1);
        oprandLoad(out, table, tableSize, i->addrs[2], t2);
        fprintf(out, "\tmul %s, %s, %s\n", t1, t1, t2);
        oprandSave(out, table, tableSize, i->addrs[0], t1);
        break;
    }
    case I_DIV:
    {
        oprandLoad(out, table, tableSize, i->addrs[1], t1);
        oprandLoad(out, table, tableSize, i->addrs[2], t2);
        fprintf(out, "\tdiv %s, %s\n", t1, t2);
        fprintf(out, "\tmflo %s\n", t1);
        oprandSave(out, table, tableSize, i->addrs[0], t1);
        break;
    }
    case I_ADDR:
    {
        NameOffsetPair e = getOffsetEntry(table, tableSize, i->addrs[1]->content.name);
        fprintf(out, "\taddi %s, $fp, %d\n", t1, e.offset);
        oprandSave(out, table, tableSize, i->addrs[0], t1);
        break;
    }
    case I_LOAD:
        oprandLoad(out, table, tableSize, i->addrs[1], t1);
        fprintf(out, "\tlw %s, 0(%s)\n", t1, t1);
        oprandSave(out, table, tableSize, i->addrs[0], t1);
        break;
    case I_SAVE:
        oprandLoad(out, table, tableSize, i->addrs[0], t1);
        oprandLoad(out, table, tableSize, i->addrs[1], t2);
        fprintf(out, "\tsw %s, 0(%s)\n", t2, t1);
        break;
    case I_GOTO:
        fprintf(out, "\tj %s\n", i->addrs[0]->content.label);
        break;
    case I_EQGOTO:
        generateGoto(out, table, tableSize, i, "beq");
        break;
    case I_NEGOTO:
        generateGoto(out, table, tableSize, i, "bne");
        break;
    case I_LTGOTO:
        generateGoto(out, table, tableSize, i, "blt");
        break;
    case I_GTGOTO:
        generateGoto(out, table, tableSize, i, "bgt");
        break;
    case I_LEGOTO:
        generateGoto(out, table, tableSize, i, "ble");
        break;
    case I_GEGOTO:
        generateGoto(out, table, tableSize, i, "bge");
        break;
    case I_RET:
        oprandLoad(out, table, tableSize, i->addrs[0], "$v0");
        fprintf(out, "\tjr $ra\n");
        break;
    case I_DEC:
        // TODO
        break;
    case I_ARG:
        // step1: push args
        // oprandLoad(out, table, tableSize, i->addrs[0], t1);
        // fprintf(out, "\tsw %s, 0($sp)\n", t1);
        // fprintf(out, "\taddi $sp $sp #4\n");
        break;
    case I_CALL:
        fprintf(out, "\tjal %s\n", i->addrs[0]->content.label);
        fprintf(out, "\tmove %s, $v0\n", t1);
        oprandSave(out, table, tableSize, i->addrs[0], t1);
        break;
    case I_PARAM:
        // TODO
        break;
    case I_READ:
        // TODO
        break;
    case I_WRITE:
        // TODO
        break;
    default:
        break;
    }
}

void generateCode(FILE* out, const IR* ir) {
    // template codes
    fprintf(out, ".globl main\n");
    fprintf(out, ".text\n");

    IRNode* i;
    // skip the first dummy node
    for (i = ir->head->next; i != NULL; i = i->next) {
        printInst(stdout, i->inst);
        printf("\n");
        generateInst(out, i);
    }
}