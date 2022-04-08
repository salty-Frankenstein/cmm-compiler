/* Type definitions */
struct Type;

typedef struct RecordField {
    char* name;                 // name of the field
    struct Type* type;          // type of the field
    struct RecordField* next;   // linked list
} RecordField;

typedef struct Record {     // for structures
    RecordField* fieldList; // a List of fields
} Record;

typedef struct Array {
    int size;
    struct Type* type;
} Array;

typedef struct Type {
    enum { PRIMITIVE, RECORD, ARRAY } tag;
    union {
        enum PrimTypeTag primitive; // for primitive types
        Record* record;             // for structures
        Array* array;               // for arrays
    } content;
} Type;

/* Symbol table definitions */
enum SymbolTag {
    S_STRUCT,           // for structure definitions
    S_VAR,              // for variable definitions
    // TODO:...
};

typedef struct SymbolTableEntry {
    enum SymbolTag tag;
    union {
        struct {
            char* name;
            Type* type;
        } structDef;    // for structure definitions
        struct {
            char* name;
            Type* type;
        } varDef;       // for variable definitions
        // TODO:...
    } content;
} SymbolTableEntry;

typedef struct SymbolTableNode {
    struct SymbolTableEntry* content;
    struct SymbolTableNode* next;
} SymbolTableNode;

typedef struct SymbolTableNode* SymbolTable;

Type* cloneType(Type* t);
RecordField* cloneFieldList(RecordField* rf);
SymbolTable getSymbleTable(Node* parseTree);


void ExtDefListHandler(Node* root, SymbolTable* table);
void ExtDefHandler(Node* root, SymbolTable* table);
RecordField* ExtDecListHandler(Node* root, Type* inputType);
Type* SpecifierHandler(Node* root, SymbolTable* table);
Type* StructSpecifierHandler(Node* root, SymbolTable* table);
RecordField* DefListHandler(Node* root, SymbolTable* table);
RecordField* DefHandler(Node* root, SymbolTable* table);
RecordField* DecListHandler(Node* root, Type* inputType);
RecordField* DecHandler(Node* root, Type* inputType);
RecordField* VarDecHandler(Node* root, Type* inputType);

void printSymbolTable(SymbolTable t);