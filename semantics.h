struct SymbolTableEntry {
  char* name;
  
};

struct SymbolTableNode {
  struct SymbolTableEntry content;
  struct SymbolTableNode* next;
};

struct SymbolTable {
  struct SymbolTableNode* table;
};

