#ifndef COMMON_H
#define COMMON_H

#ifndef NULL
#define NULL 0
#endif

typedef enum { false = 0, true = 1 } bool;

/* a macro for generating constructors */
#define CONS(retType, consName, vtag, contentType, contentField) \
    retType* consName(contentType i){\
        NEW(retType, p)\
        p->tag = vtag;\
        p->content.contentField = i;\
        return p;\
    }

#endif