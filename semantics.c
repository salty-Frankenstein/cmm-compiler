#include "parser.h"
#include "semantics.h"
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
typedef int bool;
#define true 1;
#define false 0;

#define _ TOKEN
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
    NEW(RecordField, res);
    NEW_NAME(res->name, rf->name);
    res->type = cloneType(rf->type);
    res->next = cloneFieldList(rf->next);
    return res;
}
/* type deep copy */
Type* cloneType(Type* t) {
    NEW(Type, res);
    res->tag = t->tag;
    switch (t->tag) {
    case PRIMITIVE:
        res->content.primitive = t->content.primitive;
        break;
    case RECORD:
        res->content.record->fieldList = cloneFieldList(t->content.record->fieldList);
        break;
    case ARRAY:
        res->content.array->size = t->content.array->size;
        res->content.array->type = cloneType(t->content.array->type);
        break;
    default:
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
    Node* x = GET_CHILD(root, 0);
    Node* xs = GET_CHILD(root, 1);
    ExtDefHandler(x, table);
    ExtDefListHandler(xs, table);
}

/* side effect */
void ExtDefHandler(Node* root, SymbolTable* table) {
    assert(root->tag == ExtDef);

    Node* specifier, * extDecList;
    Type* t;
    RecordField* l;
    printf("<%d>\n",root->content.nonterminal.childNum);
    fflush(stdout);
    if(PATTERN3(root, Specifier, ExtDecList, TOKEN)) { // ExtDef -> Specifier ExtDecList
        specifier = GET_CHILD(root, 0);
        extDecList = GET_CHILD(root, 1);
        SpecifierHandler(specifier, table);
        assert(extDecList->tag == TOKEN);
        l = ExtDecListHandler(extDecList, t);
        // add all definitions in the list
        for (;l != NULL; l = l->next) {
            addEntry(table, makeVarEntry(l->name, l->type));
        }
    }
    else if(PATTERN2(root, Specifier, TOKEN)) { // ExtDef -> Specifier SEMI
        specifier = GET_CHILD(root, 0);
        // side effect here, add entry if it is a struct def
        SpecifierHandler(specifier, table);
    }
    else if(PATTERN3(root, Specifier, FunDec, CompSt)) { // ExtDef -> Specifier FuncDec CompSt
        // TODO
    }
    else {
        assert(0);
    }
    // switch (root->content.nonterminal.childNum) {
    // case 1:     // ExtDef -> Specifier
    //     specifier = GET_CHILD(root, 0);
    //     // side effect here, add entry if it is a struct def
    //     SpecifierHandler(specifier, table);
    // case 2:     // ExtDef -> Specifier ExtDecList
    //     specifier = GET_CHILD(root, 0);
    //     extDecList = GET_CHILD(root, 1);
    //     SpecifierHandler(specifier, table);
    //     assert(extDecList->tag == TOKEN);
    //     l = ExtDecListHandler(extDecList, t);
    //     // add all definitions in the list
    //     for (;l != NULL; l = l->next) {
    //         addEntry(table, makeVarEntry(l->name, l->type));
    //     }
    //     break;
    // case 3:     // ExtDef -> Specifier FuncDec CompSt
    //     // TODO
    //     assert(0);
    //     break;
    // default:
    //     break;
    // }

}

RecordField* ExtDecListHandler(Node* root, Type* inputType) {
    assert(root->tag == ExtDecList);
    Node* varDec, * extDecList;
    RecordField* x, * xs;
    if(PATTERN(root, VarDec)) {
        varDec = GET_CHILD(root, 0);
        return VarDecHandler(varDec, cloneType(inputType));
    }
    else if(PATTERN3(root, VarDec, TOKEN, ExtDecList)) {
        varDec = GET_CHILD(root, 0);
        extDecList = GET_CHILD(root, 1);
        x = VarDecHandler(varDec, cloneType(inputType));
        xs = ExtDecListHandler(extDecList, inputType);
        x->next = xs;
        return x;
    }
    else {
        assert(0);
    }
    // switch (root->content.nonterminal.childNum) {
    // case 1:
    //     varDec = GET_CHILD(root, 0);
    //     return VarDecHandler(varDec, cloneType(inputType));
    // case 2:
    //     varDec = GET_CHILD(root, 0);
    //     extDecList = GET_CHILD(root, 1);
    //     x = VarDecHandler(varDec, cloneType(inputType));
    //     xs = ExtDecListHandler(extDecList, inputType);
    //     x->next = xs;
    //     return x;
    // default:
    //     assert(0);
    // }
    return NULL;
}

Type* SpecifierHandler(Node* root, SymbolTable* table) {
    assert(root->tag == Specifier);
    assert(root->content.nonterminal.childNum == 1);
    Node* child = GET_CHILD(root, 0);
    if(PATTERN(root, TOKEN)) {   // Specifier -> TYPE
        assert(child->content.terminal->tag == TYPE);
        return makePrimitiveType(child->content.terminal->content.pType);
    }
    else if(PATTERN(root, StructSpecifier)) {
        assert(child->tag == StructSpecifier);
        return StructSpecifierHandler(child, table);
    }
    else {
        assert(0);
    }
    // Node* child = GET_CHILD(root, 0);
    // if (child->tag == TOKEN) {   // Specifier -> TYPE
    //     assert(child->content.terminal->tag == TYPE);
    //     return makePrimitiveType(child->content.terminal->content.pType);
    // }
    // else {  // Specifier -> StructSpecifier
    //     assert(child->tag == StructSpecifier);
    //     return StructSpecifierHandler(child, table);
    // }
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
    if(PATTERN5(root, _, OptTag, _, DefList, _)) {
        // using defined structure
        tag = GET_CHILD(root, 0);
        assert(tag->tag == Tag);
        assert(tag->content.terminal->tag == ID);
        // lookup the tag in the table
        for (p = *table; p != NULL; p = p->next) {
            if (p->content->tag != S_STRUCT) { continue; }
            if (strcmp(tag->content.terminal->content.reprS,
                p->content->content.structDef.name) == 0) {
                // defined entry found, return a COPY type
                return cloneType(p->content->content.structDef.type);
            }
        }
        // no entry found, raise ERROR 17
        raiseError(17, root->content.nonterminal.column, "error 17");
        return NULL;
    }
    else if(PATTERN2(root, _, Tag)) {
        // define new structure
        optTag = GET_CHILD(root, 0); // XXX: nullable
        defList = GET_CHILD(root, 1); // XXX: nullable

        fieldList = DefListHandler(defList, table);
        t = makeRecordType(makeRecord(fieldList));
        if (optTag) {   // if tag exists
            assert(optTag->tag == OptTag);
            // lookup the tag in the table
            for (p = *table; p != NULL; p = p->next) {
                if (p->content->tag != S_STRUCT) { continue; }
                if (strcmp(optTag->content.terminal->content.reprS,
                    p->content->content.structDef.name) == 0) {
                    // redefinition, raise ERROR 16
                    raiseError(16, root->content.nonterminal.column, "error 16");
                    return NULL;
                }
            }
            // no entry found, define a new entry in the table
            // TODO: check ownership
            addEntry(table,
                makeStructEntry(optTag->content.terminal->content.reprS, t));
        }
        else {          // otherwise
            // TODO: what to do here?
        }
        return t;   // return the type
    }
    else {
        assert(0);
    }
    // switch (root->content.nonterminal.childNum) {
    // case 1:         // using defined structure
    //     tag = GET_CHILD(root, 0);
    //     assert(tag->tag == Tag);
    //     assert(tag->content.terminal->tag == ID);
    //     // lookup the tag in the table
    //     for (p = *table; p != NULL; p = p->next) {
    //         if (p->content->tag != S_STRUCT) { continue; }
    //         if (strcmp(tag->content.terminal->content.reprS,
    //             p->content->content.structDef.name) == 0) {
    //             // defined entry found, return a COPY type
    //             return cloneType(p->content->content.structDef.type);
    //         }
    //     }
    //     // no entry found, raise ERROR 17
    //     raiseError(17, root->content.nonterminal.column, "error 17");
    //     return NULL;
    // case 2:         // define new structure
    //     optTag = GET_CHILD(root, 0); // XXX: nullable
    //     defList = GET_CHILD(root, 1); // XXX: nullable

    //     fieldList = DefListHandler(defList, table);
    //     t = makeRecordType(makeRecord(fieldList));
    //     if (optTag) {   // if tag exists
    //         assert(optTag->tag == OptTag);
    //         // lookup the tag in the table
    //         for (p = *table; p != NULL; p = p->next) {
    //             if (p->content->tag != S_STRUCT) { continue; }
    //             if (strcmp(optTag->content.terminal->content.reprS,
    //                 p->content->content.structDef.name) == 0) {
    //                 // redefinition, raise ERROR 16
    //                 raiseError(16, root->content.nonterminal.column, "error 16");
    //                 return NULL;
    //             }
    //         }
    //         // no entry found, define a new entry in the table
    //         // TODO: check ownership
    //         addEntry(table,
    //             makeStructEntry(optTag->content.terminal->content.reprS, t));
    //     }
    //     else {          // otherwise
    //         // TODO: what to do here?
    //     }
    //     return t;   // return the type
    //     break;
    // default:
    //     assert(0);  // impossible case
    // }
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

 /* @Nullable: when argument root == NULL, return NULL
  * return a list of fields
  */
RecordField* DefListHandler(Node* root, SymbolTable* table) {
    assert(root != NULL);
    assert(root->tag == DefList);
    assert(root->content.nonterminal.childNum == 2);
    Node* def = GET_CHILD(root, 0);
    Node* defList = GET_CHILD(root, 1);
    assert(def->tag == Def);
    assert(defList->tag == DefList);
    RecordField* x = DefHandler(def, table);
    RecordField* xs = DefListHandler(defList, table);
    x->next = xs;
    return x;
}

/*
 * return a list of record fields
 */
RecordField* DefHandler(Node* root, SymbolTable* table) {
    assert(root->tag == Def);
    assert(root->content.nonterminal.childNum == 2);
    Node* specifier = GET_CHILD(root, 0);
    Node* decList = GET_CHILD(root, 1);
    assert(specifier->tag == Specifier);
    assert(decList->tag == DecList);
    Type* t = SpecifierHandler(specifier, table);
    RecordField* res = DecListHandler(decList, t);
    deleteType(t);
    return res;
}

/*
 * return a list of record fields, `inputType` is a read-only ref
 */
RecordField* DecListHandler(Node* root, Type* inputType) {
    assert(root->tag == DecList);
    Node* dec, * decList;
    RecordField* x, * xs;
    switch (root->content.nonterminal.childNum) {
    case 1:     // base case
        dec = GET_CHILD(root, 0);
        assert(dec->tag == Dec);
        return DecHandler(dec, cloneType(inputType));
    case 2:     // recursive case 
        dec = GET_CHILD(root, 0);
        decList = GET_CHILD(root, 1);
        assert(dec->tag == Dec);
        assert(decList->tag == DecList);
        x = DecHandler(dec, cloneType(inputType));
        xs = DecListHandler(decList, cloneType(inputType));
        x->next = xs;
        return x;
    default:
        assert(0);
    }
}

/*
 * XXX: this method **DO CONSUME** ownership of argument `inputType`
 */
RecordField* DecHandler(Node* root, Type* inputType) {
    assert(root->tag == Dec);
    Node* varDec = NULL;
    switch (root->content.nonterminal.childNum) {
    case 1:     // VarDec
        varDec = GET_CHILD(root, 0);
        assert(varDec->tag == VarDec);
        break;
    case 2:     // VarDec Exp
        // TODO: what to do with the Exp?
        varDec = GET_CHILD(root, 0);
        assert(varDec->tag == VarDec);
        break;
    default:
        assert(0);
    }
    assert(varDec != NULL);
    return VarDecHandler(varDec, inputType);
}

/*
 * XXX: this method **DO CONSUME** ownership of argument `inputType`
 */
RecordField* VarDecHandler(Node* root, Type* inputType) {
    assert(root->tag == VarDec);
    Node* id, *varDec, *i;
    Type* at;
    switch (root->content.nonterminal.childNum) {
    case 1:     // ID
        id = GET_CHILD(root, 0);
        assert(id->tag == TOKEN);
        return makeRecordField(
            id->content.terminal->content.reprS,
            inputType,
            NULL);  // isolated node
    case 2:     // VarDec [ Int ]
        varDec = GET_CHILD(root, 0);
        i = GET_CHILD(root, 1);
        assert(varDec->tag = VarDec);
        assert(i->tag == TOKEN);
        NEW(Array, a);
        a->size = i->content.terminal->content.intLit;
        a->type = inputType;
        at = makeArrayType(a);
        return VarDecHandler(varDec, at);   // recursively construction
    default:
        assert(0);
    }
}