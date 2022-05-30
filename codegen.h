#ifndef CODEGEN_H
#define CODEGEN_H

#include"ir.h"

void generateInst(FILE* out, const Instruction* i);
void generateCode(FILE* out, const IR* ir);

#endif