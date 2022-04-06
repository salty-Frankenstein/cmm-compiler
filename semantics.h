typedef union Type {
    enum PrimTypeTag primitive;
    // TODO: 
} Type;

typedef struct SymbolTableEntry {
    char* name;
    Type* type;
} SymbolTableEntry;

struct SymbolTableNode {
    struct SymbolTableEntry* content;
    struct SymbolTableNode* next;
};

typedef struct SymbolTableNode* SymbolTable;

SymbolTable getSymbleTable(Node* parseTree);
