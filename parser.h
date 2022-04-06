extern int errorType;

struct Token;
enum RelOpTag { LT, LE, GT, GE, EQ, NE };
struct Node* makeTokenNode(struct Token*);
struct Token* makeIntLit(int);
struct Token* makeFloatLit(float);
struct Token* makeID(char*);
struct Token* makeType(char*);
struct Token* makeRelOp(enum RelOpTag);
struct Token* makeToken(YYTOKENTYPE);