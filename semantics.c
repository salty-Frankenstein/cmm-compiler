#include "parser.h"
#include "semantics.h"
#include<assert.h>
#include<stdlib.h>

void addEntry(SymbolTable* table, SymbolTableEntry* newEntry) {
    NEW(SymbolTableNode, q);
    q->content = newEntry;
    q->next = *table;
    *table = q;
}

SymbolTable getSymbleTable(Node* root) {
    switch (root->tag) {
    case ExtDefList:
        break;
    default:
        break;
    }
    return NULL;
}

void extDefListHandler(SymbolTable* table) {

}

void SpecifierHandler(Node* root, SymbolTable* table) {
    assert(root->tag == Specifier);
    assert(root->content.nonterminal.childNum == 1);
    root->content.nonterminal.child[0];

}
