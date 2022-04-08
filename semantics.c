#include "parser.h"
#include "semantics.h"
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define _ TOKEN
#define CATCH_ALL else { assert(0); }
#define _PATTERN2(root, tag0, tag1) \
    GET_CHILD(root, 0)->tag == tag0 \
    && GET_CHILD(root, 1)->tag == tag1 

#define _PATTERN3(root, tag0, tag1, tag2) \
    _PATTERN2(root, tag0, tag1) \
    && GET_CHILD(root, 2)->tag == tag2

#define _PATTERN4(root, tag0, tag1, tag2, tag3) \
    _PATTERN3(root, tag0, tag1, tag2) \
    && GET_CHILD(root, 3)->tag == tag3

#define _PATTERN5(root, tag0, tag1, tag2, tag3, tag4) \
    _PATTERN4(root, tag0, tag1, tag2, tag3) \
    && GET_CHILD(root, 4)->tag == tag4

#define PATTERN(root, tag0) \
    root->content.nonterminal.childNum == 1 \
    && GET_CHILD(root, 0)->tag == tag0

#define PATTERN2(root, tag0, tag1) \
    root->content.nonterminal.childNum == 2 \
    && _PATTERN2(root, tag0, tag1)

#define PATTERN3(root, tag0, tag1, tag2) \
    root->content.nonterminal.childNum == 3 \
    && _PATTERN3(root, tag0, tag1, tag2)

#define PATTERN4(root, tag0, tag1, tag2, tag3) \
    root->content.nonterminal.childNum == 4 \
    && _PATTERN4(root, tag0, tag1, tag2, tag3)

#define PATTERN5(root, tag0, tag1, tag2, tag3, tag4) \
    root->content.nonterminal.childNum == 5 \
    && _PATTERN5(root, tag0, tag1, tag2, tag3, tag4)

/* error info handler */
void raiseError(int errorType, int lineNo, const char* msg) {
    assert(errorType > 0 && errorType <= 19);
    printf("Error type %d at Line %d: %s\n", errorType, lineNo, msg);
}

#define CONS_TYPE(consName, vtag, contentType, contentField) \
    Type* consName(contentType i){\
        NEW(Type, p)\
        p->tag = vtag;\
        p->content.contentField = i;\
        return p;\
    }

/* XXX: the following constructors are just wrappers
 * and **DO CONSUME** ownership of the argument
 */
CONS_TYPE(makePrimitiveType, PRIMITIVE, enum PrimTypeTag, primitive)
CONS_TYPE(makeRecordType, RECORD, Record*, record)
CONS_TYPE(makeArrayType, ARRAY, Array*, array)


/* FieldList deep copy */
RecordField* cloneFieldList(RecordField* rf) {
    if (rf == NULL) { return NULL; }
    NEW(RecordField, res);
    NEW_NAME(res->name, rf->name);
    res->type = cloneType(rf->type);
    res->next = cloneFieldList(rf->next);
    return res;
}
/* type deep copy */
Type* cloneType(Type* t) {
    if (t == NULL) { return NULL; }
    NEW(Type, res);
    res->tag = t->tag;
    switch (t->tag) {
    case PRIMITIVE:
        res->content.primitive = t->content.primitive;
        break;
    case RECORD:
        res->content.record = makeRecord(cloneFieldList(t->content.record->fieldList));
        break;
    case ARRAY:
        res->content.array = makeArray(t->content.array->size, t->content.array->type);
        break;
    default:
        printf("<%d>\n", t->tag);
        fflush(stdout);
        assert(0);
    }
    return res;
}

/* delete a type and all nested types */
void deleteType(Type* t) {
    // TODO
}

/* make symbol table entry methods, no argument ownership consumed */
SymbolTableEntry* makeStructEntry(char* name, Type* type) {
    NEW(SymbolTableEntry, res);
    res->tag = S_STRUCT;
    NEW_NAME(res->content.structDef.name, name);
    res->content.structDef.type = cloneType(type);
    return res;
}

SymbolTableEntry* makeVarEntry(char* name, Type* type) {
    NEW(SymbolTableEntry, res);
    res->tag = S_VAR;
    NEW_NAME(res->content.varDef.name, name);
    res->content.varDef.type = cloneType(type);
    return res;
}

RecordField* makeRecordField(char* name, Type* type, RecordField* next) {
    NEW(RecordField, res);
    NEW_NAME(res->name, name);
    res->type = cloneType(type);
    res->next = next;
    return res;
}

Record* makeRecord(RecordField* fieldList) {
    NEW(Record, res);
    res->fieldList = fieldList;
    return res;
}

Array* makeArray(int size, Type* type) {
    NEW(Array, res);
    res->size = size;
    res->type = cloneType(type);
    return res;
}

/* Symbol table linked list operations */
void addEntry(SymbolTable* table, SymbolTableEntry* newEntry) {
    NEW(SymbolTableNode, q);
    q->content = newEntry;
    q->next = *table;
    *table = q;
}

SymbolTable getSymbleTable(Node* root) {
    assert(root->tag == Program);
    SymbolTable res = NULL;
    ExtDefListHandler(GET_CHILD(root, 0), &res);
    return res;
}

void ExtDefListHandler(Node* root, SymbolTable* table) {
    if (root == NULL) { return; }
    assert(root->content.nonterminal.childNum == 2);
    ExtDefHandler(GET_CHILD(root, 0), table);
    ExtDefListHandler(GET_CHILD(root, 1), table);
}

/* side effect */
void ExtDefHandler(Node* root, SymbolTable* table) {
    assert(root->tag == ExtDef);

    Node* specifier, * extDecList;
    Type* t;
    RecordField* def;
    SymbolTableNode* p;
    bool flag;
    if (PATTERN3(root, Specifier, ExtDecList, _)) { // ExtDef -> Specifier ExtDecList SEMI
        specifier = GET_CHILD(root, 0);
        extDecList = GET_CHILD(root, 1);
        t = SpecifierHandler(specifier, table);
        def = ExtDecListHandler(extDecList, t);
        // add all definitions in the list
        for (;def != NULL; def = def->next) {
            // check the original table 
            for (p = *table, flag = false; p != NULL; p = p->next) {
                // if the variable has already been defined
                if (p->content->tag == S_VAR
                    && strcmp(p->content->content.varDef.name, def->name) == 0) {
                    raiseError(3, root->content.nonterminal.column, "error 3");
                    flag = true;
                    break;
                }
                // if the variable has a same name as an exist structure
                if (p->content->tag == S_STRUCT
                    && strcmp(p->content->content.structDef.name, def->name)) {
                    raiseError(3, root->content.nonterminal.column, "error 3");
                    flag = true;
                    break;
                }
            }
            if (!flag) {    // if not defined
                addEntry(table, makeVarEntry(def->name, def->type));
            }
        }
    }
    else if (PATTERN2(root, Specifier, _)) { // ExtDef -> Specifier SEMI
        specifier = GET_CHILD(root, 0);
        // side effect here, add entry if it is a struct def
        SpecifierHandler(specifier, table);
    }
    else if (PATTERN3(root, Specifier, FunDec, CompSt)) { // ExtDef -> Specifier FuncDec CompSt
        // TODO
    }
    CATCH_ALL
}

RecordField* ExtDecListHandler(Node* root, Type* inputType) {
    assert(root->tag == ExtDecList);
    Node* varDec, * extDecList;
    RecordField* x, * xs;
    if (PATTERN(root, VarDec)) {
        varDec = GET_CHILD(root, 0);
        return VarDecHandler(varDec, cloneType(inputType));
    }
    else if (PATTERN3(root, VarDec, TOKEN, ExtDecList)) {
        varDec = GET_CHILD(root, 0);
        extDecList = GET_CHILD(root, 2);
        x = VarDecHandler(varDec, cloneType(inputType));
        xs = ExtDecListHandler(extDecList, inputType);
        x->next = xs;
        return x;
    }
    CATCH_ALL;
    return NULL;
}

Type* SpecifierHandler(Node* root, SymbolTable* table) {
    assert(root->tag == Specifier);
    assert(root->content.nonterminal.childNum == 1);
    Node* child = GET_CHILD(root, 0);
    if (PATTERN(root, TOKEN)) {   // Specifier -> TYPE
        return makePrimitiveType(GET_TERMINAL(child, pType));
    }
    else if (PATTERN(root, StructSpecifier)) {
        return StructSpecifierHandler(child, table);
    }
    CATCH_ALL;
    return NULL;
}

// bool cmpStructTag(SymbolTableEntry* obj, SymbolTableEntry* sbj) {
//     assert(sbj->tag == S_STRUCT);
//     if (obj->tag != S_STRUCT) {
//         return false;
//     }
//     return strcmp(obj->content.structDef.name, sbj->content.structDef.name) == 0;
// }

/*
 * @Nullable: for error case
 * there are two kinds of Struct Specifiers:
 * for the definition, if not defined, define a structure in the symbol table
 * and return the new created record type
 * otherwise, raise ERROR 16
 * for using defined structure case, lookup the symbol table
 * - if the entry found, return a COPY of the record type
 * - otherwise, raise ERROR 17
 */
 // TODO: what if it is not a exdef? Is it necessary to add entry?
Type* StructSpecifierHandler(Node* root, SymbolTable* table) {
    assert(root->tag == StructSpecifier);
    Node* tag, * optTag, * defList;
    RecordField* fieldList;
    Type* t;
    SymbolTableNode* p;
    bool containsExp = false;
    if (PATTERN2(root, _, Tag)) {
        // using defined structure
        tag = GET_CHILD(root, 1);
        Node* id = GET_CHILD(tag, 0);
        assert(id->tag == TOKEN);
        // lookup the tag in the table
        for (p = *table; p != NULL; p = p->next) {
            if (p->content->tag != S_STRUCT) { continue; }
            if (strcmp(GET_TERMINAL(id, reprS),
                p->content->content.structDef.name) == 0) {
                // defined entry found, return a COPY type
                return cloneType(p->content->content.structDef.type);
            }
        }
        // no entry found, raise ERROR 17
        raiseError(17, root->content.nonterminal.column, "error 17");
        return NULL;
    }
    else if (root->content.nonterminal.childNum == 5) {
        // define new structure
        optTag = GET_CHILD(root, 1); // XXX: nullable
        defList = GET_CHILD(root, 3); // XXX: nullable
        if (defList != NULL) {
            fieldList = DefListHandler(defList, table, &containsExp);
            if(containsExp) {
                raiseError(15, root->content.nonterminal.column, "error 15");
                return NULL;
            }
            t = makeRecordType(makeRecord(fieldList));
        }
        else {
            t = NULL;
        }
        if (optTag) {   // if tag exists
            assert(optTag->tag == OptTag);
            // lookup the tag in the table
            Node* tag = GET_CHILD(optTag, 0);
            for (p = *table; p != NULL; p = p->next) {
                if (p->content->tag != S_STRUCT) { continue; }
                if (strcmp(GET_TERMINAL(tag, reprS),
                    p->content->content.structDef.name) == 0) {
                    // redefinition, raise ERROR 16
                    raiseError(16, root->content.nonterminal.column, "error 16");
                    return NULL;
                }
            }
            // no entry found, define a new entry in the table
            // TODO: check ownership
            SymbolTableEntry* e = makeStructEntry(GET_TERMINAL(tag, reprS), t);
            addEntry(table, e);
        }
        else {          // otherwise
            // TODO: what to do here?
        }
        return t;   // return the type
    }
    CATCH_ALL;
    return NULL;
}

/* Definition handlers */
/*
 * there two kinds of definitions: variable def & record field def
 * for variables, we check the symbol table add them into it
 * for record fields, we form a record type (which is a local symbol table)
 * here we only implement the handlers for yielding record types
 * it can be used as a local symbol table, for the variable def case
 */

 /*
  * @Nullable: return NULL, when `root` is NULL
  * return a list of fields
  */
RecordField* DefListHandler(Node* root, SymbolTable* table, bool *containsExp) {
    if (root == NULL) { return NULL; }
    assert(root->tag == DefList);
    if (root->content.nonterminal.childNum == 2) {
        Node* def = GET_CHILD(root, 0);
        Node* defList = GET_CHILD(root, 1);
        RecordField* x = DefHandler(def, table, containsExp);
        RecordField* xs = DefListHandler(defList, table, containsExp);
        x->next = xs;
        return x;
    }
    CATCH_ALL
}

/*
 * return a list of record fields
 */
RecordField* DefHandler(Node* root, SymbolTable* table, bool *containsExp) {
    assert(root->tag == Def);
    if (PATTERN3(root, Specifier, DecList, _)) {
        Node* specifier = GET_CHILD(root, 0);
        Node* decList = GET_CHILD(root, 1);
        Type* t = SpecifierHandler(specifier, table);
        RecordField* res = DecListHandler(decList, t, containsExp);
        deleteType(t);
        return res;
    }
    CATCH_ALL
}

/*
 * return a list of record fields, `inputType` is a read-only ref
 */
RecordField* DecListHandler(Node* root, Type* inputType, bool* containsExp) {
    assert(root->tag == DecList);
    Node* dec, * decList;
    RecordField* x, * xs;
    if (PATTERN(root, Dec)) {    // base case
        dec = GET_CHILD(root, 0);
        return DecHandler(dec, cloneType(inputType), containsExp);
    }
    else if (PATTERN3(root, Dec, _, DecList)) {  // recursive case
        dec = GET_CHILD(root, 0);
        decList = GET_CHILD(root, 2);
        x = DecHandler(dec, cloneType(inputType), containsExp);
        xs = DecListHandler(decList, cloneType(inputType), containsExp);
        x->next = xs;
        return x;
    }
    CATCH_ALL
}

/*
 * XXX: this method **DO CONSUME** ownership of argument `inputType`
 */
RecordField* DecHandler(Node* root, Type* inputType, bool *containsExp) {
    assert(root->tag == Dec);
    Node* varDec = NULL;
    if (PATTERN(root, VarDec)) {
        varDec = GET_CHILD(root, 0);
    }
    else if (PATTERN3(root, VarDec, _, Exp)) {
        // TODO: what to do with the Exp?
        varDec = GET_CHILD(root, 0);
        *containsExp = true;
    }
    CATCH_ALL;
    assert(varDec != NULL);
    return VarDecHandler(varDec, inputType);
}

/*
 * XXX: this method **DO CONSUME** ownership of argument `inputType`
 */
RecordField* VarDecHandler(Node* root, Type* inputType) {
    assert(root->tag == VarDec);
    Node* id, * varDec, * i;
    Type* at;
    if (PATTERN(root, TOKEN)) {  // ID
        id = GET_CHILD(root, 0);
        return makeRecordField(
            GET_TERMINAL(id, reprS),
            inputType,
            NULL);  // isolated node
    }
    else if (PATTERN4(root, VarDec, _, TOKEN, _)) { // VarDec [ Int ]
        varDec = GET_CHILD(root, 0);
        i = GET_CHILD(root, 2);
        at = makeArrayType(makeArray(GET_TERMINAL(i, intLit), inputType));
        return VarDecHandler(varDec, at);   // recursively construction

    }
    CATCH_ALL
}

/* print symbol table, for debugging */
void printIndent_(int indent) {
    int i = 0;
    for (i = 0; i < indent; i++) {
        printf("    ");
    }
}

void printType(Type* t, int indent) {
    if (t == NULL) {
        printIndent_(indent); printf("No type");
        return;
    }
    RecordField* l;
    switch (t->tag) {
    case PRIMITIVE:
        if (t->content.primitive == T_INT) {
            printIndent_(indent); printf("int\n");
        }
        else {
            printIndent_(indent); printf("float\n");
        }
        break;
    case RECORD:
        for (l = t->content.record->fieldList; l != NULL; l = l->next) {
            printIndent_(indent); printf("field name: %s\n", l->name);
            printIndent_(indent); printf("field type:\n");
            printType(l->type, indent + 1);
        }
        break;
    case ARRAY:
        printIndent_(indent); printf("array size: %d\n", t->content.array->size);
        printIndent_(indent); printf("elem type:\n");
        printType(t->content.array->type, indent + 1);
        break;
    }
}

void printSymbolTableNode(SymbolTableNode* node) {
    switch (node->content->tag) {
    case S_STRUCT:
        printf("struct name: %s\n", node->content->content.structDef.name);
        printf("type:\n");
        printType(node->content->content.structDef.type, 1);
        break;
    case S_VAR:
        printf("var name: %s\n", node->content->content.varDef.name);
        printf("type:\n");
        printType(node->content->content.varDef.type, 1);
        break;
    }
}
void printSymbolTable(SymbolTable t) {
    for (; t != NULL; t = t->next) {
        printSymbolTableNode(t);
        printf(",\n");
    }
}
