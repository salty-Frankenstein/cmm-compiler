%{
#define YYSTYPE struct Node*
#define YYERROR_VERBOSE 1
#include <stdio.h>
#include "lex.yy.c"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdarg.h>
#include "parser.h"

int errorType = 0;

#define CONS_TOKEN(consName, vtag, contentType, contentField) \
  Token* consName(contentType i){\
    NEW(Token, p)\
    p->tag = vtag;\
    p->content.contentField = i;\
    return p;\
  } \

CONS_TOKEN(makeIntLit, INT, int, intLit)
CONS_TOKEN(makeFloatLit, FLOAT, float, floatLit)
CONS_TOKEN(makeID, ID, char*, reprS)
CONS_TOKEN(makeType, TYPE, enum PrimTypeTag, pType)
CONS_TOKEN(makeRelOp, RELOP, enum RelOpTag, relOp)
Token* makeToken(enum yytokentype tag) {
  NEW(Token, p);
  p->tag = tag;
  return p;
}

Node* makeTokenNode(Token* token) {
  NEW(Node, p);
  p->tag = TOKEN;
  p->content.terminal = token;
  return p;
}

Node* makeNonterminalNode(int column, enum NodeTag tag, int childNum, ...) {
  assert(tag != TOKEN);
  NEW(Node, p);
  p->tag = tag;
  p->content.nonterminal.column = column;

  va_list valist;
  va_start(valist, childNum);

  p->content.nonterminal.childNum = childNum;
  NEW_ARRAY(Node*, pa, childNum);
  int i;
  for(i = 0; i < childNum; i++) {
    pa[i] = va_arg(valist, Node*);
  }
  p->content.nonterminal.child = pa;
  return p;
}

/* output */
void printIndent(int indent) {
  int i;
  for(i = 0; i < indent; i++) {
    printf("  ");
  }
}

void printParseTree(Node* root, int indent);
#define PRINT_TOKEN(tag) case tag: printf(#tag"\n"); break;
void printToken(Token* token, int indent) {
  printIndent(indent);

  switch (token->tag) {
  case INT: printf("INT: %d\n", token->content.intLit); break;
  case FLOAT: printf("FLOAT: %f\n", token->content.floatLit); break;
  case ID: printf("ID: "); printf("%s\n", token->content.reprS); break;
  case TYPE: 
    if(token->content.pType == T_INT) { printf("int\n"); }
    else { printf("float\n"); }
    break;
  PRINT_TOKEN(SEMI)  PRINT_TOKEN(COMMA) PRINT_TOKEN(ASSIGNOP)  PRINT_TOKEN(RELOP) 
  PRINT_TOKEN(PLUS)  PRINT_TOKEN(MINUS) PRINT_TOKEN(STAR)      PRINT_TOKEN(DIV)
  PRINT_TOKEN(AND)   PRINT_TOKEN(OR)    PRINT_TOKEN(DOT)       PRINT_TOKEN(NOT)
  PRINT_TOKEN(LP)    PRINT_TOKEN(RP)    PRINT_TOKEN(LB)        PRINT_TOKEN(RB)
  PRINT_TOKEN(LC)    PRINT_TOKEN(RC)    PRINT_TOKEN(STRUCT)    PRINT_TOKEN(RETURN)
  PRINT_TOKEN(IF)    PRINT_TOKEN(ELSE)  PRINT_TOKEN(WHILE)
  default: assert(0);
  }
}

#define PRINT_NONTERM(tag) \
  case tag: printf(#tag" (%d)\n", root->content.nonterminal.column); break;
void printNonterminal(Node* root, int indent) {
  printIndent(indent);
  assert(root->tag != TOKEN);
  switch (root->tag) {
  PRINT_NONTERM(Program)   PRINT_NONTERM(ExtDefList)      PRINT_NONTERM(ExtDef)  PRINT_NONTERM(ExtDecList)
  PRINT_NONTERM(Specifier) PRINT_NONTERM(StructSpecifier) PRINT_NONTERM(OptTag)  PRINT_NONTERM(Tag)
  PRINT_NONTERM(VarDec)    PRINT_NONTERM(FunDec)          PRINT_NONTERM(VarList) PRINT_NONTERM(ParamDec)
  PRINT_NONTERM(CompSt)    PRINT_NONTERM(StmtList)        PRINT_NONTERM(Stmt)    PRINT_NONTERM(DefList)
  PRINT_NONTERM(Def)       PRINT_NONTERM(DecList)         PRINT_NONTERM(Dec)     PRINT_NONTERM(Exp)
  PRINT_NONTERM(Args)
  default: assert(0);
  }
  int i;
  for (i = 0; i < root->content.nonterminal.childNum; i++) {
    if(root->content.nonterminal.child[i] != NULL) {
      printParseTree(root->content.nonterminal.child[i], indent + 1);
    }
  }
}

void printParseTree(Node* root, int indent) {
  if (root->tag == TOKEN) {
    printToken(root->content.terminal, indent);
  }
  else {
    printNonterminal(root, indent);
  }
}

%}

/* declared tokens */
%token INT
%token FLOAT
%token ID
%token TYPE
%token SEMI COMMA ASSIGNOP RELOP PLUS MINUS STAR DIV AND OR DOT NOT LP RP  LB RB LC RC STRUCT RETURN IF ELSE WHILE
%token BOTTOM

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%left LP RP LB RB DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%parse-param {struct Node **root}

%%
Show : Program { if(errorType == 0) { *root = $1; } }

/* High-level Definitions */
Program : ExtDefList { $$ = makeNonterminalNode(@1.first_line, Program, 1, $1); }
  ;
ExtDefList : /* empty */ { $$ = NULL; }
  | ExtDef ExtDefList { $$ = makeNonterminalNode(@1.first_line, ExtDefList, 2, $1, $2); }
  ;
ExtDef : Specifier ExtDecList SEMI { $$ = makeNonterminalNode(@1.first_line, ExtDef, 3, $1, $2, $3); }
  | Specifier SEMI { $$ = makeNonterminalNode(@1.first_line, ExtDef, 2, $1, $2); }
  | Specifier FunDec CompSt { $$ = makeNonterminalNode(@1.first_line, ExtDef, 3, $1, $2, $3); }
  | Specifier FunDec SEMI { $$ = makeNonterminalNode(@1.first_line, ExtDef, 3, $1, $2, $3); }
  | Specifier error
  ;
ExtDecList : VarDec { $$ = makeNonterminalNode(@1.first_line, ExtDecList, 1, $1); }
  | VarDec COMMA ExtDecList { $$ = makeNonterminalNode(@1.first_line, ExtDecList, 3, $1, $2, $3); }
  ;

/* Specifiers */
Specifier : TYPE { $$ = makeNonterminalNode(@1.first_line, Specifier, 1, $1); }
  | StructSpecifier { $$ = makeNonterminalNode(@1.first_line, Specifier, 1, $1); }
  ;
StructSpecifier : STRUCT OptTag LC DefList RC { $$ = makeNonterminalNode(@1.first_line, StructSpecifier, 5, $1, $2, $3, $4, $5); }
  | STRUCT Tag { $$ = makeNonterminalNode(@1.first_line, StructSpecifier, 2, $1, $2); } 
  | STRUCT error LC DefList RC
  ;
OptTag : /* empty */ { $$ = NULL; }
  | ID { $$ = makeNonterminalNode(@1.first_line, OptTag, 1, $1); }
  ;
Tag : ID { $$ = makeNonterminalNode(@1.first_line, Tag, 1, $1); }
  ;

/* Declarators */
VarDec : ID { $$ = makeNonterminalNode(@1.first_line, VarDec, 1, $1); }
  | VarDec LB INT RB { $$ = makeNonterminalNode(@1.first_line, VarDec, 4, $1, $2, $3, $4); }
  | VarDec LB error Exp RB
  ;
FunDec : ID LP VarList RP { $$ = makeNonterminalNode(@1.first_line, FunDec, 4, $1, $2, $3, $4); }
  | ID LP RP { $$ = makeNonterminalNode(@1.first_line, FunDec, 3, $1, $2, $3); }
  ;
VarList : ParamDec COMMA VarList { $$ = makeNonterminalNode(@1.first_line, VarList, 3, $1, $2, $3); }
  | ParamDec { $$ = makeNonterminalNode(@1.first_line, VarList, 1, $1); }
  ;
ParamDec : Specifier VarDec { $$ = makeNonterminalNode(@1.first_line, ParamDec, 2, $1, $2); }
  ;

/* Statements */
CompSt : LC DefList StmtList RC { $$ = makeNonterminalNode(@1.first_line, CompSt, 4, $1, $2, $3, $4); }
  | error RC
  ;
StmtList : /* empty */ { $$ = NULL; }
  | Stmt StmtList { $$ = makeNonterminalNode(@1.first_line, StmtList, 2, $1, $2); }
  ;
Stmt : Exp SEMI { $$ = makeNonterminalNode(@1.first_line, Stmt, 2, $1, $2); }
  | CompSt { $$ = makeNonterminalNode(@1.first_line, Stmt, 1, $1); }
  | RETURN Exp SEMI { $$ = makeNonterminalNode(@1.first_line, Stmt, 3, $1, $2, $3); }
  | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE { $$ = makeNonterminalNode(@1.first_line, Stmt, 5, $1, $2, $3, $4, $5); }
  | IF LP Exp RP Stmt ELSE Stmt { $$ = makeNonterminalNode(@1.first_line, Stmt, 7, $1, $2, $3, $4, $5, $6, $7); }
  | WHILE LP Exp RP Stmt { $$ = makeNonterminalNode(@1.first_line, Stmt, 5, $1, $2, $3, $4, $5); }
  | Specifier error SEMI
  | Exp error
  | error SEMI
  | RETURN error
  | IF LP error RP Stmt
  | IF LP error Stmt ELSE Stmt 
  | WHILE LP error RP Stmt 
  | WHILE LP error Stmt
  ;

/* Local Definitions */
DefList : /* empty */ { $$ = NULL; }
  | Def DefList { $$ = makeNonterminalNode(@1.first_line, DefList, 2, $1, $2); }
  ;
Def : Specifier DecList SEMI { $$ = makeNonterminalNode(@1.first_line, Def, 3, $1, $2, $3); }
  | error SEMI
  ;
DecList : Dec { $$ = makeNonterminalNode(@1.first_line, DecList, 1, $1); } 
  | Dec COMMA DecList { $$ = makeNonterminalNode(@1.first_line, DecList, 3, $1, $2, $3); }
  ;
Dec : VarDec { $$ = makeNonterminalNode(@1.first_line, Dec, 1, $1); }
  | VarDec ASSIGNOP Exp { $$ = makeNonterminalNode(@1.first_line, Dec, 3, $1, $2, $3); }
  | VarDec ASSIGNOP error
  ;

/* Expressions */
Exp : Exp ASSIGNOP Exp { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | Exp AND Exp { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | Exp OR Exp { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | Exp RELOP Exp { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | Exp PLUS Exp { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | Exp MINUS Exp { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | Exp STAR Exp { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | Exp DIV Exp { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | LP Exp RP { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | MINUS Exp %prec STAR { $$ = makeNonterminalNode(@1.first_line, Exp, 2, $1, $2); }
  | NOT Exp %prec STAR { $$ = makeNonterminalNode(@1.first_line, Exp, 2, $1, $2); }
  | ID LP Args RP { $$ = makeNonterminalNode(@1.first_line, Exp, 4, $1, $2, $3, $4); }
  | ID LP RP { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | Exp LB Exp RB { $$ = makeNonterminalNode(@1.first_line, Exp, 4, $1, $2, $3, $4); }
  | Exp DOT ID { $$ = makeNonterminalNode(@1.first_line, Exp, 3, $1, $2, $3); }
  | ID { $$ = makeNonterminalNode(@1.first_line, Exp, 1, $1); }
  | INT { $$ = makeNonterminalNode(@1.first_line, Exp, 1, $1); }
  | FLOAT { $$ = makeNonterminalNode(@1.first_line, Exp, 1, $1); }
  | Exp LB error RB
  | error RP
  | error Exp
  ;
Args : Exp COMMA Args { $$ = makeNonterminalNode(@1.first_line, Args, 3, $1, $2, $3); }
  | Exp { $$ = makeNonterminalNode(@1.first_line, Args, 1, $1); }
  ;

%%
yyerror(Node* _, char* msg) {
  if(errorType != 1) {  // ignore type A error here
    printf("Error Type B at Line %d\n", yylineno);
    // printf("Error type B at Line %d:%s\n", yylineno, msg);
    // fprintf(stderr, "error: %s\n", msg);
  } 
  errorType = 2;
}