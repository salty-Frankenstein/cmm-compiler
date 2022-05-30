#include "codegen.h"
#include<assert.h>

void printInst(FILE* out, const Instruction* i);

// allocate registers for varibles
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

void generateInst(FILE* out, const Instruction* i) {
    switch (i->tag) {
    case I_LABEL:
        fprintf(out, "%s:\n", i->addrs[0]->content.label);
        break;
    case I_FUNC:
        // TODO
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
        generateInst(out, i->inst);
    }
}