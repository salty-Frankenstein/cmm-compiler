%{
#include "syntax.tab.h"
#include <string.h>
#include "parser.h"

int yycolumn = 1;
#define YY_USER_ACTION \
  yylloc.first_line = yylloc.last_line = yylineno; \
  yylloc.first_column = yycolumn; \
  yylloc.last_column = yycolumn + yyleng - 1; \
  yycolumn += yyleng;
%}
nonzero [1-9]
digit [0-9]
digits {digit}+
letter [_a-zA-Z]
integer 0|{nonzero}{digit}*
normalFloat {digits}\.{digits}
decimal {normalFloat}|\.{digits}|{digits}\.
signed [\+-]?{digits}
expFloat {decimal}[eE]{signed}

blank " "|\t|\r
%%
{blank} {}
\n { yycolumn = 1; yylineno++; }

{integer} {
  yylval = makeTokenNode(makeIntLit(atoi(yytext)));
  return INT;
}

{normalFloat}|{expFloat} {
  yylval = makeTokenNode(makeFloatLit(atof(yytext)));
  return FLOAT;
}

; {
  yylval = makeTokenNode(makeToken(SEMI));
  return SEMI;
}

, {
  yylval = makeTokenNode(makeToken(COMMA));
  return COMMA;
}

= {
  yylval = makeTokenNode(makeToken(ASSIGNOP));
  return ASSIGNOP;
}

\< {
  yylval = makeTokenNode(makeRelOp(LT));
  return RELOP;
}

\<= {
  yylval = makeTokenNode(makeRelOp(LE));
  return RELOP;
}

\> {
  yylval = makeTokenNode(makeRelOp(GT));
  return RELOP;
}

\>= {
  yylval = makeTokenNode(makeRelOp(GE));
  return RELOP;
}

== {
  yylval = makeTokenNode(makeRelOp(EQ));
  return RELOP;
}

!= {
  yylval = makeTokenNode(makeRelOp(NE));
  return RELOP;
}

\+ {
  yylval = makeTokenNode(makeToken(PLUS));
  return PLUS;
}

- {
  yylval = makeTokenNode(makeToken(MINUS));
  return MINUS;
}

\* {
  yylval = makeTokenNode(makeToken(STAR));
  return STAR;
}

\/ {
  yylval = makeTokenNode(makeToken(DIV));
  return DIV;
}

&& {
  yylval = makeTokenNode(makeToken(AND));
  return AND;
}

\|\| {
  yylval = makeTokenNode(makeToken(OR));
  return OR;
}

\. {
  yylval = makeTokenNode(makeToken(DOT));
  return DOT;
}

! {
  yylval = makeTokenNode(makeToken(NOT));
  return NOT;
}

int {
  yylval = makeTokenNode(makeType(T_INT));
  return TYPE;
}

float {
  yylval = makeTokenNode(makeType(T_FLOAT));
  return TYPE;
}

\( {
  yylval = makeTokenNode(makeToken(LP));
  return LP;
}

\) {
  yylval = makeTokenNode(makeToken(RP));
  return RP;
}

\[ {
  yylval = makeTokenNode(makeToken(LB));
  return LB;
}

\] {
  yylval = makeTokenNode(makeToken(RB));
  return RB;
}

\{ {
  yylval = makeTokenNode(makeToken(LC));
  return LC;
}

\} {
  yylval = makeTokenNode(makeToken(RC));
  return RC;
}

struct {
  yylval = makeTokenNode(makeToken(STRUCT));
  return STRUCT;
}

return {
  yylval = makeTokenNode(makeToken(RETURN));
  return RETURN;
}

if {
  yylval = makeTokenNode(makeToken(IF));
  return IF;
}

else {
  yylval = makeTokenNode(makeToken(ELSE));
  return ELSE;
}

while {
  yylval = makeTokenNode(makeToken(WHILE));
  return WHILE;
}

{letter}+({digit}|{letter})* {
  char* p = (char*)malloc(100);
  strcpy(p, yytext);
  yylval = makeTokenNode(makeID(p));
  return ID;
}

. { 
  printf("Error Type A at Line %d\n", yylineno);
  // printf("Error type A at Line %d: Mysterious character %s\n", yylineno, yytext);
  errorType = 1;
  return BOTTOM;
}

%%
