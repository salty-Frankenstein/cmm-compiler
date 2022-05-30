#include<stdio.h>
#include<stdlib.h>
#include "syntax.tab.h"
#include "parser.h"
#include "semantics.h"
#include "ir.h"
#include "codegen.h"

extern FILE* yyin;

int main(int argc, char** argv) {
    if (argc <= 2) { return 1; }
    FILE* f = fopen(argv[1], "r");
    FILE* outFile = fopen(argv[2], "w");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    if (!outFile) {
        perror(argv[2]);
        return 1;
    }
    yyrestart(f);
    Node* root;
    yyparse(&root);
    if (errorType == 0) {
        // printParseTree(root, 0);
        SymbolTable t = getSymbleTable(root);
        // printSymbolTable(t);
        if (!semanticsError) {
            IR* ir = makeIR();
            translateProgram(ir, root, t);
            // printIR(outFile, ir);
            generateCode(outFile, ir);
        }
    }
    fclose(f);
    fclose(outFile);
    return 0;
}

/*

int main(int argc, char** argv) {
    if (argc > 1) {
        if (! (yyin = fopen(argv[1], "r")) ) {
            perror(argv[1]);
            return 1;
        }
    }
    while (yylex() != 0);
    return 0;
}

*/