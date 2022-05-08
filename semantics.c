#include "parser.h"
#include "semantics.h"
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

/* error info handler */
void raiseError(int errorType, int lineNo, char* msg) {
    assert(errorType > 0 && errorType <= 19);
    printf("Error type %d at Line %d: %s\n", errorType, lineNo, msg);
}

/* XXX: the following constructors are just wrappers
 * and **DO CONSUME** ownership of the argument
 */
CONS(Type, makePrimitiveType, PRIMITIVE, enum PrimTypeTag, primitive)
CONS(Type, makeRecordType, RECORD, Record*, record)
CONS(Type, makeArrayType, ARRAY, Array*, array)
CONS(Type, makeFuncType, FUNC, FuncSignature*, func)

CONS(SymbolTableEntry, makeStructEntry, S_STRUCT, NameTypePair*, structDef)
CONS(SymbolTableEntry, makeVarEntry, S_VAR, NameTypePair*, varDef)
CONS(SymbolTableEntry, makeFieldEntry, S_FIELD, NameTypePair*, varDef)
CONS(SymbolTableEntry, makeFuncEntry, S_FUNC, FunctionEntry*, funcDef)

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
        res->content.record = makeRecord(t->content.record->name,
            cloneFieldList(t->content.record->fieldList));
        break;
    case ARRAY:
        res->content.array = makeArray(t->content.array->size, t->content.array->type);
        break;
    case FUNC:
        res->content.func = makeFuncSignature(t->content.func->retType,
            t->content.func->params);
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

/* get base type and dimension information of an array type */
void arrayInfo(Type* array, enum PrimTypeTag* baseType, int* dimension) {
    assert(array->tag == ARRAY);
    if (array->content.array->type->tag == PRIMITIVE) {
        *baseType = array->content.array->type->content.primitive;
        *dimension = 0;
    }
    else {
        arrayInfo(array->content.array->type, baseType, dimension);
        *dimension++;
    }
}

// TODO: test this function
/* t1 and t2 are not NULL */
bool typeEqual(Type* t1, Type* t2) {
    if (t1->tag != t2->tag) {
        return false;
    }
    enum PrimTypeTag b1, b2;
    int d1, d2;
    switch (t1->tag) {
    case PRIMITIVE:
        return t1->content.primitive == t2->content.primitive;
    case RECORD:    // name equivalence
        if (t1->content.record->name == NULL || t2->content.record->name == NULL) {
            return false;
        }
        return strcmp(t1->content.record->name, t2->content.record->name) == 0;
    case ARRAY:
        // compare the base type and dimension
        arrayInfo(t1, &b1, &d1); arrayInfo(t2, &b2, &d2);
        return b1 == b2 && d1 == d2;
    default:
        assert(0);
    }
}

// TODO: test this function
// check two function signatures
bool sameSignature(FuncSignature s1, FuncSignature s2) {
    Type* retT1 = s1.retType, * retT2 = s2.retType;
    RecordField* p1 = s1.params, * p2 = s2.params;
    if (!typeEqual(retT1, retT2)) {
        return false;
    }
    for (; p1 != NULL && p2 != NULL; p1 = p1->next, p2 = p2->next) {
        if (!typeEqual(p1->type, p2->type)) {
            return false;
        }
    }
    return p1 == NULL && p2 == NULL;
}

// TODO: check all calls for ownership
/* consume `type` */
NameTypePair* makeNameTypePair(char* name, Type* type) {
    NEW(NameTypePair, res);
    NEW_NAME(res->name, name);
    res->type = type;
    return res;
}

/* DO **CONSUME** the `retType` and `params` */
FuncSignature* makeFuncSignature(Type* retType, RecordField* params) {
    NEW(FuncSignature, res);
    res->retType = retType;
    res->params = params;
    return res;
}

FunctionEntry* makeFunctionEntry(char* name, Type* t, bool defined, int lineNo) {
    assert(t->tag == FUNC);
    NEW(FunctionEntry, res);
    NEW_NAME(res->name, name);
    res->type = cloneType(t);
    res->defined = defined;
    res->decLineNo = lineNo;
    return res;
}

// TODO: ownership
RecordField* makeRecordField(char* name, Type* type, RecordField* next) {
    NEW(RecordField, res);
    res->name = name;
    // NEW_NAME(res->name, name);
    res->type = cloneType(type);
    res->next = next;
    return res;
}

Record* makeRecord(char* name, RecordField* fieldList) {
    NEW(Record, res);
    if (name == NULL) {
        res->name == NULL;
    }
    else {
        NEW_NAME(res->name, name);
    }
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

// check all functions is well-defined
void checkFuncDef(SymbolTable table) {
    SymbolTableNode* p;
    for (p = table; p != NULL; p = p->next) {
        if (p->content->tag == S_FUNC
            && !p->content->content.funcDef->defined) {
            raiseError(18, p->content->content.funcDef->decLineNo, "error 18");
        }
    }
}

SymbolTable getSymbleTable(Node* root) {
    assert(root->tag == Program);
    SymbolTable res = NULL;
    ExtDefListHandler(GET_CHILD(root, 0), &res);
    checkFuncDef(res);
    return res;
}

void ExtDefListHandler(Node* root, SymbolTable* table) {
    if (root == NULL) { return; }
    assert(root->content.nonterminal.childNum == 2);
    ExtDefHandler(GET_CHILD(root, 0), table);
    ExtDefListHandler(GET_CHILD(root, 1), table);
}

// `isField` for field definition
void defineVar(RecordField* def, SymbolTable* table, int lineNo, bool isField) {
    SymbolTableNode* p;
    bool flag;
    // check the original table 
    for (p = *table, flag = false; p != NULL; p = p->next) {
        // if the variable has already been defined
        if (p->content->tag == S_VAR
            && strcmp(p->content->content.varDef->name, def->name) == 0) {
            raiseError(3, lineNo, "error 3");
            flag = true;
            break;
        }
        // if field is already defined
        if (isField && p->content->tag == S_FIELD
            && strcmp(p->content->content.varDef->name, def->name) == 0) {
            raiseError(15, lineNo, "error 15");
            flag = true;
            break;
        }
        // if the variable has a same name as an exist structure
        if (p->content->tag == S_STRUCT
            && strcmp(p->content->content.structDef->name, def->name) == 0) {
            raiseError(3, lineNo, "error 3");
            flag = true;
            break;
        }
        // TODO: what if var-func conflict?
    }
    if (!flag) {    // if not defined
        if (isField) {
            addEntry(table, makeFieldEntry(makeNameTypePair(def->name, def->type)));
        }
        else {
            addEntry(table, makeVarEntry(makeNameTypePair(def->name, def->type)));
        }
    }
}

/* side effect */
void ExtDefHandler(Node* root, SymbolTable* table) {
    assert(root->tag == ExtDef);

    Node* specifier, * extDecList, * funDec, * compSt;
    Type* t, * retType;
    RecordField* def;
    SymbolTableNode* p;
    bool flag;
    if (PATTERN3(root, Specifier, ExtDecList, _)) { // ExtDef -> Specifier ExtDecList SEMI
        specifier = GET_CHILD(root, 0);
        extDecList = GET_CHILD(root, 1);
        t = SpecifierHandler(specifier, table);
        // side effect here, add entries
        def = ExtDecListHandler(extDecList, t);
        defineVar(def, table, GET_LINENO(root), false);

    }
    else if (PATTERN2(root, Specifier, _)) { // ExtDef -> Specifier SEMI
        specifier = GET_CHILD(root, 0);
        // side effect here, add entry if it is a struct def
        SpecifierHandler(specifier, table);
    }
    else if (PATTERN3(root, Specifier, FunDec, CompSt)) { // function definition
        specifier = GET_CHILD(root, 0);
        funDec = GET_CHILD(root, 1);
        compSt = GET_CHILD(root, 2);
        // FIXME: side effect here, is it reasonable?
        retType = SpecifierHandler(specifier, table);
        FunDecHandler(funDec, table, retType, true);
        CompStHandler(compSt, table, retType);
    }
    else if (PATTERN3(root, Specifier, FunDec, TOKEN)) { // function declaration 
        specifier = GET_CHILD(root, 0);
        funDec = GET_CHILD(root, 1);
        compSt = GET_CHILD(root, 2);
        // FIXME: side effect here, is it reasonable?
        retType = SpecifierHandler(specifier, table);
        FunDecHandler(funDec, table, retType, false);
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
                p->content->content.structDef->name) == 0) {
                // defined entry found, return a COPY type
                return cloneType(p->content->content.structDef->type);
            }
        }
        // no entry found, raise ERROR 17
        raiseError(17, GET_LINENO(root), "error 17");
        return NULL;
    }
    else if (root->content.nonterminal.childNum == 5) {
        // define new structure
        optTag = GET_CHILD(root, 1); // XXX: nullable
        defList = GET_CHILD(root, 3); // XXX: nullable
        fieldList = DefListHandler(defList, table, &containsExp, true);
        if (containsExp) {
            raiseError(15, GET_LINENO(root), "error 15");
            return NULL;
        }
        // TODO: maybe refractor here
        if (optTag) {
            char* name = GET_TERMINAL(GET_CHILD(optTag, 0), reprS);
            t = makeRecordType(makeRecord(name, fieldList));
        }
        else {
            t = makeRecordType(makeRecord(NULL, fieldList));
        }
        if (optTag) {   // if tag exists
            assert(optTag->tag == OptTag);
            // lookup the tag in the table
            Node* tag = GET_CHILD(optTag, 0);
            for (p = *table; p != NULL; p = p->next) {
                if (p->content->tag != S_STRUCT) { continue; }
                if (strcmp(GET_TERMINAL(tag, reprS),
                    p->content->content.structDef->name) == 0) {
                    // redefinition, raise ERROR 16
                    raiseError(16, GET_LINENO(root), "error 16");
                    return NULL;
                }
            }
            // no entry found, define a new entry in the table
            // TODO: check ownership
            SymbolTableEntry* e = makeStructEntry(makeNameTypePair(GET_TERMINAL(tag, reprS), t));
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
RecordField* DefListHandler(Node* root, SymbolTable* table, bool* containsExp, bool isField) {
    if (root == NULL) { return NULL; }
    assert(root->tag == DefList);
    if (root->content.nonterminal.childNum == 2) {
        Node* def = GET_CHILD(root, 0);
        Node* defList = GET_CHILD(root, 1);
        RecordField* x = DefHandler(def, table, containsExp, isField);
        // note that x is a list of record fields
        // thus the xs should be appended to the tail of x, instead of `next`
        RecordField* xs = DefListHandler(defList, table, containsExp, isField);
        RecordField* tail = x;
        while(tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = xs;
        return x;
    }
    CATCH_ALL
}

/*
 * @side effect
 * return a list of record fields
 */
RecordField* DefHandler(Node* root, SymbolTable* table, bool* containsExp, bool isField) {
    assert(root->tag == Def);
    if (PATTERN3(root, Specifier, DecList, _)) {
        Node* specifier = GET_CHILD(root, 0);
        Node* decList = GET_CHILD(root, 1);
        Type* t = SpecifierHandler(specifier, table);
        RecordField* res = DecListHandler(decList, table, t, containsExp, isField);
        deleteType(t);
        return res;
    }
    CATCH_ALL
}

/*
 * @side effect: this method define the corresponding var list to the symbol table
 * return a list of record fields, `inputType` is a read-only ref
 */
RecordField* DecListHandler(Node* root, SymbolTable* table, Type* inputType, bool* containsExp, bool isField) {
    assert(root->tag == DecList);
    Node* dec, * decList;
    RecordField* x, * xs;
    if (PATTERN(root, Dec)) {    // base case
        dec = GET_CHILD(root, 0);
        return DecHandler(dec, table, cloneType(inputType), containsExp, isField);
    }
    else if (PATTERN3(root, Dec, _, DecList)) {  // recursive case
        dec = GET_CHILD(root, 0);
        decList = GET_CHILD(root, 2);
        x = DecHandler(dec, table, cloneType(inputType), containsExp, isField);
        xs = DecListHandler(decList, table, cloneType(inputType), containsExp, isField);
        x->next = xs;
        return x;
    }
    CATCH_ALL
}

/*
 * @side effect: this method define the corresponding var to the symbol table
 * XXX: this method **DO CONSUME** ownership of argument `inputType`
 */
RecordField* DecHandler(Node* root, SymbolTable* table, Type* inputType, bool* containsExp, bool isField) {
    assert(root->tag == Dec);
    Node* varDec = NULL;
    if (PATTERN(root, VarDec)) {
        varDec = GET_CHILD(root, 0);
    }
    else if (PATTERN3(root, VarDec, _, Exp)) {
        // TODO: what to do with the Exp?
        varDec = GET_CHILD(root, 0);
        Type* t = ExpHandler(GET_CHILD(root, 2), *table);
        *containsExp = true;
    }
    CATCH_ALL;
    assert(varDec != NULL);

    RecordField* def = VarDecHandler(varDec, inputType);
    defineVar(def, table, GET_LINENO(root), isField);
    return def;
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
        return makeRecordField(GET_TERMINAL(id, reprS), inputType, NULL);  // isolated node
    }
    else if (PATTERN4(root, VarDec, _, TOKEN, _)) { // VarDec [ Int ]
        varDec = GET_CHILD(root, 0);
        i = GET_CHILD(root, 2);
        at = makeArrayType(makeArray(GET_TERMINAL(i, intLit), inputType));
        return VarDecHandler(varDec, at);   // recursively construction
    }
    CATCH_ALL
}

/*
 * This handler is for both definitions & declarations with the arg `isDec`
 * DO CONSUME `retType`
 */
void FunDecHandler(Node* root, SymbolTable* table, Type* retType, bool isDef) {
    assert(root->tag == FunDec);
    Node* id, * varList;
    RecordField* paramList;
    char* funcName;
    if (PATTERN4(root, TOKEN, _, VarList, _)) {
        id = GET_CHILD(root, 0);
        funcName = GET_TERMINAL(id, reprS);
        varList = GET_CHILD(root, 2);
        paramList = VarListHandler(varList, table, isDef);
    }
    else if (PATTERN3(root, TOKEN, _, _)) {
        id = GET_CHILD(root, 0);
        funcName = GET_TERMINAL(id, reprS);
        paramList = NULL;
    }
    CATCH_ALL;

    SymbolTableNode* p;
    // first check the table
    for (p = *table; p != NULL; p = p->next) {
        if (p->content->tag == S_FUNC) {
            if (strcmp(p->content->content.funcDef->name, funcName) == 0) {
                // if entry with same name found
                if (isDef && p->content->content.funcDef->defined) {
                    // def-def conflict
                    raiseError(4, GET_LINENO(root), "error 4");
                    return;
                }
                // no def-def conflict, check the signature
                FuncSignature temp;
                temp.params = paramList;
                temp.retType = retType;
                if (sameSignature(temp,
                    *(p->content->content.funcDef->type->content.func))) {     // if matched
                    if (isDef) {    // if it is a definition, since there's no conflict
                        assert(!p->content->content.funcDef->defined);
                        // define this entry
                        p->content->content.funcDef->defined = true;
                        return;
                    }
                    else {          // if it is a declaration
                        return;     // then do nothing
                    }
                }
                else {
                    raiseError(19, GET_LINENO(root), "error 19");
                    return;
                }
            }
        }
        else if (p->content->tag == S_VAR
            && strcmp(p->content->content.varDef->name, funcName) == 0) {
            // this error is not required
            raiseError(-1, GET_LINENO(root), "additional error");
            return;
        }
    }
    // no previous entry found, then add a new entry
    Type* funcType = makeFuncType(makeFuncSignature(retType, paramList));
    addEntry(table, makeFuncEntry(makeFunctionEntry(funcName, funcType, isDef, GET_LINENO(root))));
}

/*
 * return a param field list
 */
RecordField* VarListHandler(Node* root, SymbolTable* table, bool isDef) {
    assert(root->tag == VarList);
    RecordField* x, * xs;
    if (PATTERN3(root, ParamDec, _, VarList)) { // recursive case
        x = ParamDecHandler(GET_CHILD(root, 0), table, isDef);
        xs = VarListHandler(GET_CHILD(root, 2), table, isDef);
        x->next = xs;
        return x;
    }
    else if (PATTERN(root, ParamDec)) {         // base case
        return ParamDecHandler(GET_CHILD(root, 0), table, isDef);
    }
    CATCH_ALL
}

/*
 * return a param field
 * the parameters will be defined only when the function is defined
 */
RecordField* ParamDecHandler(Node* root, SymbolTable* table, bool isDef) {
    assert(root->tag == ParamDec);
    Node* specifier, * varDec;
    Type* t;
    RecordField* field;
    if (PATTERN2(root, Specifier, VarDec)) {
        specifier = GET_CHILD(root, 0);
        varDec = GET_CHILD(root, 1);
        // FIXME: side effect here
        t = SpecifierHandler(specifier, table);
        field = VarDecHandler(varDec, t);
        if (isDef) {
            defineVar(field, table, GET_LINENO(root), false);
        }
        return field;
    }
    CATCH_ALL
}

/*
 * statements & expressions
 * since all variables are global, this method contains side effect
 */
void CompStHandler(Node* root, SymbolTable* table, Type* retType) {
    assert(root->tag == CompSt);
    if (root->content.nonterminal.childNum == 4) {   // { DefList StmtList }
    // if (PATTERN4(root, _, DefList, StmtList, _)) {
        bool waste;
        DefListHandler(GET_CHILD(root, 1), table, &waste, false);
        StmtListHandler(GET_CHILD(root, 2), table, retType);
    }
    CATCH_ALL
}

void StmtListHandler(Node* root, SymbolTable* table, Type* retType) {
    if (root == NULL) { return; }
    assert(root->tag == StmtList);
    StmtHandler(GET_CHILD(root, 0), table, retType);
    StmtListHandler(GET_CHILD(root, 1), table, retType);
}

void StmtHandler(Node* root, SymbolTable* table, Type* retType) {
    assert(root->tag == Stmt);
    if (PATTERN2(root, Exp, _)) {        // Exp;
        // check the expression
        Type* t = ExpHandler(GET_CHILD(root, 0), *table);
        deleteType(t);
    }
    else if (PATTERN(root, CompSt)) {
        CompStHandler(GET_CHILD(root, 0), table, retType);
    }
    else if (PATTERN3(root, _, Exp, _)) {    // return Exp;
        Type* t = ExpHandler(GET_CHILD(root, 1), *table);
        if (t != NULL && !typeEqual(t, retType)) {
            raiseError(8, GET_LINENO(root), "error 8");
        }
        deleteType(t);
    }
    else if (PATTERN5(root, _, _, Exp, _, Stmt)) {   // if(Exp) Stmt || while(Exp) Stmt
        Type* t = ExpHandler(GET_CHILD(root, 2), *table);
        StmtHandler(GET_CHILD(root, 4), table, retType);
        deleteType(t);
    }
    else if (PATTERN7(root, _, _, Exp, _, Stmt, _, Stmt)) {  // if(Exp) Stmt else Stmt
        Type* t = ExpHandler(GET_CHILD(root, 2), *table);
        StmtHandler(GET_CHILD(root, 4), table, retType);
        StmtHandler(GET_CHILD(root, 6), table, retType);
        deleteType(t);
    }
    CATCH_ALL
}

/*
 * a helper function to check if a expr is lval
 */
bool isLValue(Node* exp) {
    assert(exp->tag == Exp);
    if (PATTERN(exp, _)) {
        return GET_CHILD(exp, 0)->content.terminal->tag == ID;
    }
    else if (PATTERN4(exp, Exp, _, Exp, _)) {
        return true;
    }
    else if (PATTERN3(exp, Exp, _, _)) {
        return true;
    }
    return false;
}

// TODO: make sure that all sub exprs are visited
/*
 * @Nullable, for error case
 * pure, for type checking
 * return type of the expression if checked
 */
Type* ExpHandler(Node* root, SymbolTable table) {
    assert(root->tag == Exp);
    if (PATTERN3(root, Exp, _, Exp)) {       // binary
        Type* t1 = ExpHandler(GET_CHILD(root, 0), table);
        Type* t2 = ExpHandler(GET_CHILD(root, 2), table);
        if (t1 == NULL || t2 == NULL) {
            // TODO: does it need to be an error here?
            return NULL;    // just a Maybe Monad
        }
        switch (GET_CHILD(root, 1)->content.terminal->tag) {
        case ASSIGNOP:
            // check if the lhs is lval
            if (!isLValue(GET_CHILD(root, 0))) {
                raiseError(6, GET_LINENO(root), "error 6");
                goto assignErr;
            }
            // match type
            if (typeEqual(t1, t2)) {
                deleteType(t1);
                return t2;
            }
            else {
                raiseError(5, GET_LINENO(root), "error 5");
                goto assignErr;
            }
        assignErr:
            deleteType(t1); deleteType(t2);
            return NULL;
        case AND: case OR:
            // logic expr is only for int
            if (t1->tag == PRIMITIVE && t2->tag == PRIMITIVE
                && t1->content.primitive == T_INT && t2->content.primitive == T_INT) {
                deleteType(t1); deleteType(t2);
                return makePrimitiveType(T_INT);
            }
            break;
        case RELOP:
            // relation operators, only for int-int or float-float
            // but returns int
            if (t1->tag == PRIMITIVE && t2->tag == PRIMITIVE
                && t1->content.primitive == t2->content.primitive) {
                deleteType(t1); deleteType(t2);
                return makePrimitiveType(T_INT);
            }
            break;
        case PLUS: case MINUS: case STAR: case DIV:
            // arithmetic operators, only for int-int or float-float
            if (t1->tag == PRIMITIVE && t2->tag == PRIMITIVE
                && t1->content.primitive == t2->content.primitive) {
                deleteType(t1); deleteType(t2);
                return makePrimitiveType(t1->content.primitive);
            }
            break;
        default:
            assert(0);
        }
        // oprand type not matched
        deleteType(t1); deleteType(t2);
        raiseError(7, GET_LINENO(root), "error 7");
        return NULL;
    }
    else if (PATTERN3(root, _, Exp, _)) {    // (Exp)
        // no additional checking
        return ExpHandler(GET_CHILD(root, 1), table);
    }
    else if (PATTERN2(root, _, Exp)) {       // -Exp & !Exp
        Type* t = ExpHandler(GET_CHILD(root, 1), table);
        if (t == NULL) {
            return NULL;
        }
        switch (GET_CHILD(root, 0)->content.terminal->tag) {
        case MINUS:
            if (t->tag == PRIMITIVE) {
                return t;
            }
            break;
        case NOT:
            if (t->tag == PRIMITIVE && t->content.primitive == T_INT) {
                return t;
            }
            break;
        default:
            assert(0);
        }
        deleteType(t);
        raiseError(7, GET_LINENO(root), "error 7");
        return NULL;
    }
    else if (PATTERN4(root, _, _, Args, _)) {    // ID(Args)
        Type* t = IDHandler(GET_CHILD(root, 0), table, ID_FUNC, GET_LINENO(root));
        RecordField* args = ArgsHandler(GET_CHILD(root, 2), table);
        if (t == NULL || args == NULL) {
            goto callArgsErr;
        }
        // check signature
        if (t->tag == FUNC) {
            if (t->content.func->params == NULL) {
                raiseError(9, GET_LINENO(root), "error 9");
                goto callArgsErr;
            }
            else {
                RecordField* a = args;
                RecordField* p = t->content.func->params;
                for (; a != NULL && p != NULL; a = a->next, p = p->next) {
                    if (!typeEqual(a->type, p->type)) {
                        raiseError(9, GET_LINENO(root), "error 9");
                        goto callArgsErr;
                    }
                }
                if (a == NULL && p == NULL) {
                    deleteType(t);
                    // TODO: delete args
                    return cloneType(t->content.func->retType);
                }
                else {
                    raiseError(9, GET_LINENO(root), "error 9");
                    goto callArgsErr;
                }
            }
        }
        else {
            raiseError(11, GET_LINENO(root), "error 11");
            goto callArgsErr;
        }
    callArgsErr:
        deleteType(t);
        return NULL;
    }
    else if (PATTERN3(root, _, _, _)) {      // ID()
        Type* t = IDHandler(GET_CHILD(root, 0), table, ID_FUNC, GET_LINENO(root));
        if (t == NULL) {
            return NULL;
        }
        // check signature
        if (t->tag == FUNC) {
            if (t->content.func->params == NULL) {
                Type* retType = cloneType(t->content.func->retType);
                deleteType(t);
                return retType;
            }
            else {
                deleteType(t);
                raiseError(9, GET_LINENO(root), "error 9");
                return NULL;
            }
        }
        else {
            deleteType(t);
            raiseError(11, GET_LINENO(root), "error 11");
            return NULL;
        }
    }
    else if (PATTERN4(root, Exp, _, Exp, _)) {   // Exp[Exp]
        Type* t1 = ExpHandler(GET_CHILD(root, 0), table);
        Type* t2 = ExpHandler(GET_CHILD(root, 2), table);
        if (t1 == NULL || t2 == NULL) {
            return NULL;
        }
        if (t1->tag == ARRAY) {
            if (t2->tag == PRIMITIVE && t2->content.primitive == T_INT) {
                Type* retType = cloneType(t1->content.array->type);
                deleteType(t1); deleteType(t2);
                return retType;
            }
            else {
                deleteType(t1); deleteType(t2);
                raiseError(12, GET_LINENO(root), "error 12");
                return NULL;
            }
        }
        else {
            deleteType(t1); deleteType(t2);
            raiseError(10, GET_LINENO(root), "error 10");
            return NULL;
        }
    }
    else if (PATTERN3(root, Exp, _, _)) {    // Exp.ID
        Type* t1 = ExpHandler(GET_CHILD(root, 0), table);
        if (t1 == NULL) {
            return NULL;
        }
        if (t1->tag == RECORD) {
            Type* t2 = IDRecHandler(GET_CHILD(root, 2), t1->content.record, GET_LINENO(root));
            if (t2 == NULL) {
                return NULL;
            }
            return t2;
        }
        else {
            deleteType(t1);
            raiseError(13, GET_LINENO(root), "error 13");
            return NULL;
        }
    }
    else if (PATTERN(root, TOKEN)) {         // lit & id, base case
        Node* token = GET_CHILD(root, 0);
        switch (token->content.terminal->tag) {
        case ID:
            return IDHandler(token, table, ID_VAR, GET_LINENO(root));
        case INT:
            return makePrimitiveType(T_INT);
        case FLOAT:
            return makePrimitiveType(T_FLOAT);
        default:
            assert(0);
        }
    }
    CATCH_ALL
}

/*
 * @Nullable, for error case
 * quite special one, for ID (in expressions)
 */
Type* IDHandler(Node* root, SymbolTable table, enum IDKind kind, int lineNo) {
    assert(root->tag == TOKEN && root->content.terminal->tag == ID);
    assert(kind == ID_VAR || kind == ID_FUNC);
    SymbolTableNode* p;
    /* lookup the table and return the type */
    for (p = table; p != NULL; p = p->next) {
        switch (p->content->tag) {
        case S_VAR:
            if (strcmp(p->content->content.varDef->name,
                GET_TERMINAL(root, reprS)) == 0) {
                return cloneType(p->content->content.varDef->type);
            }
            break;
        case S_FUNC:
            if (strcmp(p->content->content.funcDef->name,
                GET_TERMINAL(root, reprS)) == 0) {
                return cloneType(p->content->content.funcDef->type);
            }
            break;
        default:
            continue;   // skip the structure since it's a type
            // TODO: what if the var name is same as struct name?
        }
    }
    // not found, raise error
    switch (kind) {
    case ID_VAR:
        raiseError(1, lineNo, "error 1");
        break;
    case ID_FUNC:
        raiseError(2, lineNo, "error 2");
        break;
    default:
        assert(0);
    }
    return NULL;
}

/*
 * @Nullable, for error case
 * same as above, but a record instead of symbol table
 * for structures
 */
Type* IDRecHandler(Node* root, Record* record, int lineNo) {
    assert(root->tag == TOKEN && root->content.terminal->tag == ID);
    RecordField* p;
    for (p = record->fieldList; p != NULL; p = p->next) {
        if (strcmp(p->name, GET_TERMINAL(root, reprS)) == 0) {
            return cloneType(p->type);
        }
    }
    raiseError(14, lineNo, "error 14");
    return NULL;
}

/**
 * @Nullable, for error case
 * return a list of record fields, with no names
 */
RecordField* ArgsHandler(Node* root, SymbolTable table) {
    assert(root->tag == Args);
    if (PATTERN(root, Exp)) {
        Type* t = ExpHandler(GET_CHILD(root, 0), table);
        if (t == NULL) {
            return NULL;
        }
        else {
            // TODO: lifetime here
            return makeRecordField(NULL, t, NULL);
        }
    }
    else if (PATTERN3(root, Exp, _, Args)) {
        // assert(0);
        Type* t = ExpHandler(GET_CHILD(root, 0), table);
        if (t == NULL) {
            return NULL;
        }
        else {
            RecordField* xs = ArgsHandler(GET_CHILD(root, 2), table);
            if (xs == NULL) {
                return NULL;
            }
            else {
                RecordField* x = makeRecordField(NULL, t, xs);
                return x;
            }
        }
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
        printIndent_(indent); printf("No type\n");
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
        printIndent_(indent); printf("name: %s\n", t->content.record->name);
        l = t->content.record->fieldList;
        if (l == NULL) {
            printIndent_(indent); printf("empty record\n");
        }
        else {
            for (; l != NULL; l = l->next) {
                printIndent_(indent); printf("field name: %s\n", l->name);
                printIndent_(indent); printf("field type:\n");
                printType(l->type, indent + 1);
            }
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
    RecordField* p;
    switch (node->content->tag) {
    case S_STRUCT:
        printf("struct name: %s\n", node->content->content.structDef->name);
        printf("type:\n");
        printType(node->content->content.structDef->type, 1);
        break;
    case S_VAR:
        printf("var name: %s\n", node->content->content.varDef->name);
        printf("type:\n");
        printType(node->content->content.varDef->type, 1);
        break;
    case S_FUNC:
        printf("func name: %s\n", node->content->content.funcDef->name);
        printf("return type:\n");
        printType(node->content->content.funcDef->type->content.func->retType, 1);
        if (node->content->content.funcDef->type->content.func->params == NULL) {
            printf("no params\n");
            break;
        }
        else {
            printf("params:\n");
            for (p = node->content->content.funcDef->type->content.func->params;
                p != NULL; p = p->next) {
                printIndent_(1); printf("para name: %s\n", p->name);
                printIndent_(1); printf("para type:\n");
                printType(p->type, 2);
            }
            break;
        }
    case S_FIELD:
        printf("field name: %s\n", node->content->content.varDef->name);
        printf("type:\n");
        printType(node->content->content.varDef->type, 1);
        break;
    }

}
void printSymbolTable(SymbolTable t) {
    printf("-------------------\n");
    for (; t != NULL; t = t->next) {
        printSymbolTableNode(t);
        printf("-------------------\n");
    }
}
