#include<stdio.h>
#include<stdlib.h>
#include "syntax.tab.h"

extern FILE* yyin;
extern int errorType;

int main(int argc, char** argv) {
    if (argc <= 1) { return 1; }
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    struct Node *root;
    yyparse(&root);
    if(errorType == 0) {
        printParseTree(root, 0);
    }
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