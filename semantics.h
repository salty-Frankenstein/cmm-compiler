#include "common.h"
enum IDKind { ID_VAR, ID_FIELD, ID_FUNC };

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

typedef struct FuncSignature {
    struct Type* retType;
    RecordField* params;
} FuncSignature;

typedef struct Type {
    enum { PRIMITIVE, RECORD, ARRAY, FUNC } tag;
    union {
        enum PrimTypeTag primitive; // for primitive types
        Record* record;             // for structures
        Array* array;               // for arrays
        FuncSignature* func;        // for functions
    } content;
} Type;

/* Symbol table definitions */
enum SymbolTag {
    S_STRUCT,           // for structure definitions
    S_VAR,              // for variable definitions
    S_FUNC,             // for function def & dec
    S_FIELD,            // for field def, since field is also global
    // TODO:...
};

typedef struct NameTypePair {
    char* name;
    Type* type;
} NameTypePair;

typedef struct FunctionEntry {
    char* name;
    Type* type;
    bool defined;   // if the function has been defined
    int decLineNo;  // the line number of the first declaration
} FunctionEntry;

typedef struct SymbolTableEntry {
    enum SymbolTag tag;
    union {
        NameTypePair* structDef;    // for structure definitions
        NameTypePair* varDef;       // for variable definitions
        FunctionEntry* funcDef;      // for function def & dec
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
FunctionEntry* makeFunctionEntry(char* name, Type* t, bool defined, int lineNo);
FuncSignature* makeFuncSignature(Type* retType, RecordField* params);

void ExtDefListHandler(Node* root, SymbolTable* table);
void ExtDefHandler(Node* root, SymbolTable* table);
RecordField* ExtDecListHandler(Node* root, Type* inputType);
Type* SpecifierHandler(Node* root, SymbolTable* table);
Type* StructSpecifierHandler(Node* root, SymbolTable* table);
RecordField* DefListHandler(Node* root, SymbolTable* table, bool* containsExp, bool isField);
RecordField* DefHandler(Node* root, SymbolTable* table, bool* containsExp, bool isField);
RecordField* DecListHandler(Node* root, SymbolTable* table, Type* inputType, bool* containsExp, bool isField);
RecordField* DecHandler(Node* root, SymbolTable* table, Type* inputType, bool* containsExp, bool isField);
RecordField* VarDecHandler(Node* root, Type* inputType);

void FunDecHandler(Node* root, SymbolTable* table, Type* retType, bool isDef);
RecordField* VarListHandler(Node* root, SymbolTable* table, bool isDef);
RecordField* ParamDecHandler(Node* root, SymbolTable* table, bool isDef);

void CompStHandler(Node* root, SymbolTable* table, Type* retType);
void StmtListHandler(Node* root, SymbolTable* table, Type* retType);
void StmtListHandler(Node* root, SymbolTable* table, Type* retType);
void StmtHandler(Node* root, SymbolTable* table, Type* retType);
Type* ExpHandler(Node* root, SymbolTable table);
Type* IDHandler(Node* root, SymbolTable table, enum IDKind kind, int lineNo);
Type* IDRecHandler(Node* root, Record* record, int lineNo);
RecordField* ArgsHandler(Node* root, SymbolTable table);

void printSymbolTable(SymbolTable t);
void printIndent_(int indent);
void printType(Type* t, int indent);