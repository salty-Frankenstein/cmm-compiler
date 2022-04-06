#ifndef PARSER_H
#define PARSER_H

#include "syntax.tab.h"

#define NEW(varType, varName) \
  struct varType* varName = (struct varType*)malloc(sizeof(struct varType));
#define NEW_ARRAY(varType, varName, size) struct varType* varName = (struct varType*)malloc(size * sizeof(struct varType));

extern int errorType;

enum PrimTypeTag { T_INT, T_FLOAT };
enum RelOpTag { LT, LE, GT, GE, EQ, NE };
/* Tokens */
struct Token {
  enum yytokentype tag;
  union {
    int intLit;             // for INT
    float floatLit;         // for FLOAT
    char* reprS;            // for ID 
    enum PrimTypeTag pType; // for TYPE
    enum RelOpTag relOp;    // for RELOP
  } content;
};
typedef struct Token Token;

/* Parse Tree Node */
enum NodeTag {
    TOKEN,
    Program, ExtDefList, ExtDef, ExtDecList,  // High-level Definitions
    Specifier, StructSpecifier, OptTag, Tag,  // Specifiers
    VarDec, FunDec, VarList, ParamDec,        // Declarators
    CompSt, StmtList, Stmt,                   // Statements
    DefList, Def, DecList, Dec,               // Local Definitions
    Exp, Args                                 // Expressions
};

struct Node {
    enum NodeTag tag;
    union {
        struct Token* terminal;
        struct {
            int childNum;
            struct Node** child;
            int column;
        } nonterminal;  // array of child nodes
    } content;
};
typedef struct Node Node;

struct Node* makeTokenNode(struct Token*);
struct Token* makeIntLit(int);
struct Token* makeFloatLit(float);
struct Token* makeID(char*);
struct Token* makeType(enum PrimTypeTag);
struct Token* makeRelOp(enum RelOpTag);
struct Token* makeToken(YYTOKENTYPE);
void printParseTree(struct Node* root, int indent);

#ifndef NULL
#define NULL 0
#endif

#endif