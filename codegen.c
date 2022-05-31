#include "codegen.h"
#include<assert.h>
#include<string.h>

void printInst(FILE* out, const Instruction* i);

// print the offset table for debugging
void printOffsetTable(const NameOffsetPair* table, int size) {
    printf("\ttable size %d\n", size);
    int i;
    for (i = 0; i < size; i++) {
        printf("\tvar: %s, offset: %d\n", table[i].name, table[i].offset);
    }
}

// allocate registers for varibles
// all the variable info is from the var-offset table
char* getReg(Oprand* var) {
    // printf("%d\n", var->tag);
    // fflush(stdout);
    assert(var->tag == OP_VAR);
    // TODO: complete this function
    static int no = 0;
    char* res = (char*)malloc(NAME_SIZE);
    sprintf(res, "$%s", var->content.name);
    // sprintf(res, "$t%d", no);
    no++;
    return res;
}

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
                        table[*out_size].offset = var_info.offset;
                        firstVar = false;
                    }
                    else {
                        table[*out_size].offset = var_info.offset
                            + table[*out_size - 1].offset;
                    }
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

// since literal values are not available in many instructions
// preparations are needed
void litHandler(FILE* out, const Instruction* i) {
    switch (i->tag) {
    case I_SUB:
        if (i->addrs[1]->tag == OP_LIT) {
            fprintf(out, "li $temp1, %d\n", i->addrs[1]->content.lit);
            fprintf(out, "sub %s, $temp1, %s\n", getReg(i->addrs[0]), getReg(i->addrs[2]));
        }
        else {
            assert(0);
        }
        break;
    case I_MUL:
        // note costant folding here, there is at most one Lit
        if (i->addrs[1]->tag == OP_LIT) {
            fprintf(out, "li $temp1, %d\n", i->addrs[1]->content.lit);
            fprintf(out, "mul %s, $temp1, %s\n", getReg(i->addrs[0]), getReg(i->addrs[2]));
        }
        else if (i->addrs[2]->tag == OP_LIT) {
            fprintf(out, "li $temp2, %d\n", i->addrs[2]->content.lit);
            fprintf(out, "mul %s, $temp2, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]));
        }
        else {
            assert(i->addrs[1]->tag == OP_VAR && i->addrs[2]->tag == OP_VAR);
            fprintf(out, "mul %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), getReg(i->addrs[2]));
        }
        break;
    case I_DIV:
        if (i->addrs[1]->tag == OP_LIT && i->addrs[2]->tag == OP_LIT) {
            fprintf(out, "li $temp1, %d\n", i->addrs[1]->content.lit);
            fprintf(out, "li $temp2, %d\n", i->addrs[2]->content.lit);
            fprintf(out, "div $temp1, $temp2\n");
            fprintf(out, "mflo %s\n", getReg(i->addrs[0]));
        }
        else if (i->addrs[1]->tag == OP_LIT) {
            fprintf(out, "li $temp1, %d\n", i->addrs[1]->content.lit);
            fprintf(out, "div $temp1, %s\n", getReg(i->addrs[2]));
            fprintf(out, "mflo %s\n", getReg(i->addrs[0]));
        }
        else if (i->addrs[2]->tag == OP_LIT) {
            fprintf(out, "li $temp2, %d\n", i->addrs[2]->content.lit);
            fprintf(out, "div %s, $temp2\n", getReg(i->addrs[1]));
            fprintf(out, "mflo %s\n", getReg(i->addrs[0]));
        }
        else {
            assert(i->addrs[1]->tag == OP_VAR && i->addrs[2]->tag == OP_VAR);
            fprintf(out, "div %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), getReg(i->addrs[2]));
        }
        break;
    default:
        assert(0);
    }
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
        printOffsetTable(table, tableSize);
        break;
    case I_ASSGN:
        if (i->addrs[1]->tag == OP_LIT) {
            fprintf(out, "li %s, %d\n", getReg(i->addrs[0]),
                i->addrs[1]->content.lit);
        }
        else {
            assert(i->addrs[1]->tag == OP_VAR);
            fprintf(out, "move %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]));
        }
        break;
    case I_ADD:
        // since constant folding is performed, then there would be at most 1 lit-op
        if (i->addrs[2]->tag == OP_LIT) {
            fprintf(out, "addi %s, %s, %d\n", getReg(i->addrs[0]), getReg(i->addrs[1]), i->addrs[2]->content.lit);
        }
        else {
            if (i->addrs[1]->tag == OP_LIT) {
                fprintf(out, "addi %s, %s, %d\n", getReg(i->addrs[0]), getReg(i->addrs[2]), i->addrs[1]->content.lit);
            }
            else {
                fprintf(out, "add %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), getReg(i->addrs[2]));
            }
        }
        break;
    case I_SUB:
        if (i->addrs[2]->tag == OP_LIT) {
            fprintf(out, "addi %s, %s, %d\n", getReg(i->addrs[0]), getReg(i->addrs[1]), -i->addrs[2]->content.lit);
        }
        else {
            if (i->addrs[1]->tag == OP_LIT) {
                litHandler(out, i);
            }
            else {
                fprintf(out, "sub %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), getReg(i->addrs[2]));
            }
        }
        break;
    case I_MUL:
        litHandler(out, i);
        break;
    case I_DIV:
        litHandler(out, i);
        // fprintf(out, "div %s, %s\n", getReg(i->addrs[1]), getReg(i->addrs[2]));
        // fprintf(out, "mflo %s\n", getReg(i->addrs[0]));
        break;
    case I_ADDR:    // TODO
        break;
    case I_LOAD:
        fprintf(out, "lw %s, 0(%s)\n", getReg(i->addrs[0]), getReg(i->addrs[1]));
        break;
    case I_SAVE:
        fprintf(out, "sw %s, 0(%s)\n", getReg(i->addrs[0]), getReg(i->addrs[1]));
        break;
    case I_GOTO:
        fprintf(out, "j %s\n", i->addrs[0]->content.label);
        break;
    case I_EQGOTO:
        fprintf(out, "beq %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), i->addrs[2]->content.label);
        break;
    case I_NEGOTO:
        fprintf(out, "bne %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), i->addrs[2]->content.label);
        break;
    case I_LTGOTO:
        fprintf(out, "blt %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), i->addrs[2]->content.label);
        break;
    case I_GTGOTO:
        fprintf(out, "bgt %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), i->addrs[2]->content.label);
        break;
    case I_LEGOTO:
        fprintf(out, "ble %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), i->addrs[2]->content.label);
        break;
    case I_GEGOTO:
        fprintf(out, "bge %s, %s, %s\n", getReg(i->addrs[0]), getReg(i->addrs[1]), i->addrs[2]->content.label);
        break;
    case I_RET:
        if (i->addrs[0]->tag == OP_LIT) {
            fprintf(out, "li $v0, %d\n", i->addrs[0]->content.lit);
        }
        else {
            assert(i->addrs[0]->tag == OP_VAR);
            fprintf(out, "move $v0, %s\n", getReg(i->addrs[0]));
        }
        fprintf(out, "jr $ra\n");
        break;
    case I_DEC:
        // TODO
        break;
    case I_ARG:
        // TODO
        break;
    case I_CALL:
        fprintf(out, "jal %s\n", i->addrs[0]->content.label);
        fprintf(out, "move %s, $v0\n", getReg(i->addrs[0]));
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