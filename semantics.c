#include "parser.h"
#include "semantics.h"

SymbolTable* getSymbleTable(Node* root) {
    switch (root->tag) {
    case ExtDefList:
        break;
    default:
        break;
    }
    return NULL;
}

void extDefListHandler(SymbolTable** table) {

}