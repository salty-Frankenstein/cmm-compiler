#include "codegen.h"
#include<assert.h>
#include<string.h>

const char* t1 = "$t1";
const char* t2 = "$t2";

void printInst(FILE* out, const Instruction* i);

// print the offset table for debugging
void printOffsetTable(const OffsetTable table) {
    printf("\ttable size %d\n", table.size);
    printf("\tpara num: %d\n", table.paramnum);
    int i;
    for (i = 0; i < table.size; i++) {
        printf("\tvar: %s, offset: %d\n", table.table[i].name, table.table[i].offset);
    }
    printf("\tthe end offset is: %d\n", table.table[table.size].offset);
}

/*
    oldfp-> +-----------+
            | ret addr  |
            +-----------+
            | auto vars |
            +-----------+
            | ...       |
            +-----------+
            | argn...   |
    $fp+8-> +-----------+
            | arg1      |
    $fp+4-> +-----------+
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
    8. $fp <- 4($fp)
    9. jump
after return:
    10. $ra <- 0($sp)
    11. $sp <- $sp + 4 * (arg num + 1)
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
OffsetTable makeFuncVarTable(const IRNode* begin, const IRNode* end) {
    int len = 0;
    const IRNode* p;
    // the length of the function, an upper bound of var num
    for (p = begin; p != end; p = p->next) {
        len++;
    }

    NameOffsetPair* table;
    int size = 0;
    int paramnum = 0;
    table = (NameOffsetPair*)malloc(len * sizeof(NameOffsetPair));

    bool firstVar = true;
    int lastOffset = 0;
    for (p = begin; p != end; p = p->next) {
        NameOffsetPair var_info = getDefVar(p->inst);
        if (var_info.name != NULL) {
            int i;
            bool in_table = false;
            for (i = 0; i < size; i++) {
                if (strcmp(table[i].name, var_info.name) == 0) {
                    in_table = true;
                    break;
                }
            }

            // push a new variable into the table 
            if (!in_table) {
                table[size].name = var_info.name;
                // HACK: note that the PARAM declarations should always at the front
                // the offset field is a prefix sum
                // params' offsets are positive, and variables' are negative
                if (var_info.isparam) {
                    if (size == 0) {
                        // set the start address of the parameters
                        table[size].offset = 8;
                    }
                    else {
                        table[size].offset = lastOffset
                            + table[size - 1].offset;
                    }
                    paramnum++;
                }
                else {
                    if (firstVar) {
                        // set the start address of the variables
                        table[size].offset = -4;
                        firstVar = false;
                    }
                    else {
                        table[size].offset = lastOffset
                            + table[size - 1].offset;
                    }
                    // HACK: note that the stack grows to the lower address
                    // but arrays grows to the higher
                    // here is a hack to make array var point to it's first element
                    // the adjustment has to be done after the next offset is set
                    if (-lastOffset > 4) { // the last entry is an array
                        table[size-1].offset = table[size].offset + 4;
                    }
                }
                lastOffset = var_info.offset;
                size++;
            }
        }
    }

    // HACK: the last one is a dummy entry with name=NULL, 
    // and offset=the end of all local vars, for $sp initialization
    table[size].name = NULL;
    if (size == 0) {
        table[size].offset = 0;
    }
    else {
        table[size].offset = lastOffset + table[size - 1].offset;
    }

    // then all the variables are collected, the no replicated
    OffsetTable res;
    res.table = table;
    res.size = size;
    res.paramnum = paramnum;
    res.ismain = strcmp(begin->inst->addrs[0]->content.label, "main") == 0;
    return res;
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

NameOffsetPair getOffsetEntry(const OffsetTable table, const char* name) {
    int i;
    for (i = 0; i < table.size; i++) {
        if (strcmp(table.table[i].name, name) == 0) {
            return table.table[i];
        }
    }
    assert(0);
}

// generate code for var op & lit which needs to be loaded to a reg
// and return the reg (string)
// since instructions have at most 2 oprands
// `pos` decides which reg to choose
void oprandLoad(FILE* out, const OffsetTable table, Oprand* op, const char* reg) {
    assert(op->tag != OP_LABEL);
    if (op->tag == OP_LIT) {
        fprintf(out, "\tli %s, %d\n", reg, op->content.lit);
    }
    else {
        NameOffsetPair e = getOffsetEntry(table, op->content.name);
        // then load offset($fp) to the destination
        fprintf(out, "\tlw %s, %d($fp)\n", reg, e.offset);
    }
}

void oprandSave(FILE* out, const OffsetTable table, Oprand* op, const char* reg) {
    assert(op->tag == OP_VAR);
    NameOffsetPair e = getOffsetEntry(table, op->content.name);
    fprintf(out, "\tsw %s, %d($fp)\n", reg, e.offset);
}

void generateGoto(FILE* out, const OffsetTable table, const Instruction* i, char* inst) {
    oprandLoad(out, table, i->addrs[0], t1);
    oprandLoad(out, table, i->addrs[1], t2);
    fprintf(out, "\t%s %s, %s, %s\n", inst, t1, t2, i->addrs[2]->content.label);
}

// the ad hoc code for read write functions
void initSyscall(FILE* out) {
    fprintf(out, "read:\n\tli $v0, 4\n\tla $a0, _prompt\n\tsyscall\n\tli $v0, 5\n\tsyscall\n\tjr $ra\n");
    fprintf(out, "write:\n\tli $v0, 1\n\tsyscall\n\tli $v0, 4\n\tla $a0, _ret\n\tsyscall\n\tmove $v0, $0\n\tjr $ra\n");
}

// this function is designed to be called in the instruction order only
// calls out of order would lead to unexpected behaviors
void generateInst(FILE* out, const IRNode* irn) {
    // the table is managed by this function itself
    // it updates the table when it enters a new function,
    // and delete the old one
    // for a pseudo one-pass implementation
    static OffsetTable table;
    static bool is_first = true;
    const Instruction* i = irn->inst;

    switch (i->tag) {
    case I_LABEL:
        fprintf(out, "%s:\n", i->addrs[0]->content.label);
        break;
    case I_FUNC:
        // first clean the old table
        if (is_first) { is_first = false; }
        else { free(table.table); }
        table = makeFuncVarTable(irn, getFunctionEnd(irn));
        // init func here
        fprintf(out, "\n%s:\n", i->addrs[0]->content.label);
        if (table.ismain) {
            // HACK: $fp initialization is needed
            // since the ret addr is not needed for main, 
            // we can cancel this by adding the offset back to $fp
            fprintf(out, "\taddi $fp, $sp, %d\n", 4);
        }
        // step 6: push auto vars
        fprintf(out, "\taddi $sp, $fp, %d\n", table.table[table.size].offset);
        // printOffsetTable(table);
        break;
    case I_ASSGN:
        oprandLoad(out, table, i->addrs[1], t1);
        oprandSave(out, table, i->addrs[0], t1);
        break;
    case I_ADD:
        // since constant folding is performed, then there would be at most 1 lit-op
        if (i->addrs[2]->tag == OP_LIT) {
            oprandLoad(out, table, i->addrs[1], t1);
            fprintf(out, "\taddi %s, %s, %d\n", t1, t1, i->addrs[2]->content.lit);
            oprandSave(out, table, i->addrs[0], t1);
        }
        else {
            if (i->addrs[1]->tag == OP_LIT) {
                oprandLoad(out, table, i->addrs[2], t1);
                fprintf(out, "\taddi %s, %s, %d\n", t1, t1, i->addrs[1]->content.lit);
                oprandSave(out, table, i->addrs[0], t1);
            }
            else {
                oprandLoad(out, table, i->addrs[1], t1);
                oprandLoad(out, table, i->addrs[2], t2);
                fprintf(out, "\tadd %s, %s, %s\n", t1, t1, t2);
                oprandSave(out, table, i->addrs[0], t1);
            }
        }
        break;
    case I_SUB:
        if (i->addrs[2]->tag == OP_LIT) {
            oprandLoad(out, table, i->addrs[1], t1);
            fprintf(out, "\taddi %s, %s, %d\n", t1, t1, -i->addrs[2]->content.lit);
            oprandSave(out, table, i->addrs[0], t1);
        }
        else {
            oprandLoad(out, table, i->addrs[1], t1);
            oprandLoad(out, table, i->addrs[2], t2);
            fprintf(out, "\tsub %s, %s, %s\n", t1, t1, t2);
            oprandSave(out, table, i->addrs[0], t1);
        }
        break;
    case I_MUL:
    {
        oprandLoad(out, table, i->addrs[1], t1);
        oprandLoad(out, table, i->addrs[2], t2);
        fprintf(out, "\tmul %s, %s, %s\n", t1, t1, t2);
        oprandSave(out, table, i->addrs[0], t1);
        break;
    }
    case I_DIV:
    {
        oprandLoad(out, table, i->addrs[1], t1);
        oprandLoad(out, table, i->addrs[2], t2);
        fprintf(out, "\tdiv %s, %s\n", t1, t2);
        fprintf(out, "\tmflo %s\n", t1);
        oprandSave(out, table, i->addrs[0], t1);
        break;
    }
    case I_ADDR:
    {
        NameOffsetPair e = getOffsetEntry(table, i->addrs[1]->content.name);
        fprintf(out, "\taddi %s, $fp, %d\n", t1, e.offset);
        oprandSave(out, table, i->addrs[0], t1);
        break;
    }
    case I_LOAD:
        oprandLoad(out, table, i->addrs[1], t1);
        fprintf(out, "\tlw %s, 0(%s)\n", t1, t1);
        oprandSave(out, table, i->addrs[0], t1);
        break;
    case I_SAVE:
        oprandLoad(out, table, i->addrs[0], t1);
        oprandLoad(out, table, i->addrs[1], t2);
        fprintf(out, "\tsw %s, 0(%s)\n", t2, t1);
        break;
    case I_GOTO:
        fprintf(out, "\tj %s\n", i->addrs[0]->content.label);
        break;
    case I_EQGOTO:
        generateGoto(out, table, i, "beq");
        break;
    case I_NEGOTO:
        generateGoto(out, table, i, "bne");
        break;
    case I_LTGOTO:
        generateGoto(out, table, i, "blt");
        break;
    case I_GTGOTO:
        generateGoto(out, table, i, "bgt");
        break;
    case I_LEGOTO:
        generateGoto(out, table, i, "ble");
        break;
    case I_GEGOTO:
        generateGoto(out, table, i, "bge");
        break;
    case I_RET:
        if (table.ismain) {
            fprintf(out, "\tmove $v0, $0\n\tjr $ra\n");
        }
        else {
            // step 7: $sp <- $fp
            fprintf(out, "\tmove $sp, $fp\n");
            // note that load is depends on $fp
            oprandLoad(out, table, i->addrs[0], "$v0");
            // step 8: recover $fp
            fprintf(out, "\tlw $fp, 4($fp)\n");
            // step 9: jump
            fprintf(out, "\tjr $ra\n");
        }
        break;
    case I_DEC:
        // do nothing
        break;
    case I_ARG:
        // step1: push args, but leave $sp unset
        // sw arg_i, ((i+1-n)*4)($sp)
    {
        int offset_to_sp = (i->addrs[1]->content.lit + 1 - table.paramnum) * 4;
        oprandLoad(out, table, i->addrs[0], t1);
        fprintf(out, "\tsw %s, %d($sp)\n", t1, offset_to_sp);
        break;
    }
    case I_CALL:
        // step1 cont.: set $sp <- $sp - 4*-(n+1), for params & old fp
        fprintf(out, "\taddi $sp, $sp, %d\n", 4 * -(table.paramnum + 1));
        // step2: push $fp
        fprintf(out, "\tsw $fp, 4($sp)\n");
        // step3: $fp <- $sp
        fprintf(out, "\tmove $fp, $sp\n");
        // step4: push $ra
        fprintf(out, "\tsw $ra, 0($fp)\n");
        fprintf(out, "\taddi $sp, $sp, -4\n");
        // step5: jump
        fprintf(out, "\tjal %s\n", i->addrs[1]->content.label);
        // step 10: recover $ra
        fprintf(out, "\tlw $ra, 0($sp)\n");
        // step 11: pop old fp & args
        fprintf(out, "\taddi $sp, $sp, %d\n", 4 * (table.paramnum + 1));
        fprintf(out, "\tmove %s, $v0\n", t1);
        oprandSave(out, table, i->addrs[0], t1);
        break;
    case I_PARAM:
        // do nothing
        break;
        // the implementation for read & write is quite ad hoc
    case I_READ:
        fprintf(out, "\taddi $sp, $sp, -4\n\tsw $ra, 0($sp)\n\tjal read\n"
            "\tlw $ra, 0($sp)\n\taddi $sp, $sp, 4\n");
        oprandSave(out, table, i->addrs[0], "$v0");
        break;
    case I_WRITE:
        oprandLoad(out, table, i->addrs[0], "$a0");
        fprintf(out, "\taddi $sp, $sp, -4\n\tsw $ra, 0($sp)\n\tjal write\n"
            "\tlw $ra, 0($sp)\n\taddi $sp, $sp, 4\n");
        break;
    default:
        break;
    }
}

void generateCode(FILE* out, const IR* ir) {
    // template codes
    fprintf(out, ".data\n_prompt: .asciiz \"Enter an integer:\"\n");
    fprintf(out, "_ret: .asciiz \"\\n\"\n");
    fprintf(out, ".globl main\n.text\n");
    initSyscall(out);

    IRNode* i;
    // skip the first dummy node
    for (i = ir->head->next; i != NULL; i = i->next) {
        // printInst(stdout, i->inst);
        // printf("\n");
        generateInst(out, i);
    }
}