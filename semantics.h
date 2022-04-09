typedef int bool;
#define true 1
#define false 0

/* Type definitions */
struct Type;

typedef struct RecordField {
    char* name;                 // name of the field
    struct Type* type;          // type of the field
    struct RecordField* next;   // linked list
} RecordField;

typedef struct Record {     // for structures
    char* name;             // all record types have a name
                            // for anonymous records, this field is NULL
    RecordField* fieldList; // a List of fields
} Record;

typedef struct Array {
    int size;
    struct Type* type;
} Array;

/* since functions are not first class,
 * we don't include function types here
 */
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
    S_FUNC,             // for function def & dec
    // TODO:...
};

typedef struct NameTypePair {
    char* name;
    Type* type;
} NameTypePair;

typedef struct FuncSignature {
    char* name;
    Type* retType;
    RecordField* params;
    bool defined;
} FuncSignature;

// TODO: separate the nested structrues out of the definition
typedef struct SymbolTableEntry {
    enum SymbolTag tag;
    union {
        NameTypePair* structDef;    // for structure definitions
        NameTypePair* varDef;       // for variable definitions
        FuncSignature* funcDef;      // for function def & dec
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

Record* makeRecord(char* name, RecordField* fieldList);
Array* makeArray(int size, Type* type);

void ExtDefListHandler(Node* root, SymbolTable* table);
void ExtDefHandler(Node* root, SymbolTable* table);
RecordField* ExtDecListHandler(Node* root, Type* inputType);
Type* SpecifierHandler(Node* root, SymbolTable* table);
Type* StructSpecifierHandler(Node* root, SymbolTable* table);
RecordField* DefListHandler(Node* root, SymbolTable* table, bool* containsExp);
RecordField* DefHandler(Node* root, SymbolTable* table, bool* containsExp);
RecordField* DecListHandler(Node* root, Type* inputType, bool* containsExp);
RecordField* DecHandler(Node* root, Type* inputType, bool* containsExp);
RecordField* VarDecHandler(Node* root, Type* inputType);

void FunDecHandler(Node* root, SymbolTable* table, Type* retType, bool isDef);
RecordField* VarListHandler(Node* root, SymbolTable* table);
RecordField* ParamDecHandler(Node* root, SymbolTable* table);


void printSymbolTable(SymbolTable t);
void printIndent_(int indent);
void printType(Type* t, int indent);