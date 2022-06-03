#ifndef CODEGEN_H
#define CODEGEN_H

#include"ir.h"

// the function variable offset table entry struct
typedef struct NameOffsetPair {
    char* name;
    int offset;
    bool isparam;
} NameOffsetPair;

typedef struct OffsetTable {
    NameOffsetPair* table;
    int size;
    int paramnum;   // number of parameter of the function
} OffsetTable;

void generateInst(FILE* out, const IRNode* i);
void generateCode(FILE* out, const IR* ir);

#endif