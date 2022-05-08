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

#define _PATTERN6(root, tag0, tag1, tag2, tag3, tag4, tag5) \
    _PATTERN5(root, tag0, tag1, tag2, tag3, tag4) \
    && GET_CHILD(root, 5)->tag == tag5

#define _PATTERN7(root, tag0, tag1, tag2, tag3, tag4, tag5, tag6) \
    _PATTERN6(root, tag0, tag1, tag2, tag3, tag4, tag5) \
    && GET_CHILD(root, 6)->tag == tag6

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

#define PATTERN6(root, tag0, tag1, tag2, tag3, tag4, tag5) \
    root->content.nonterminal.childNum == 6 \
    && _PATTERN6(root, tag0, tag1, tag2, tag3, tag4, tag5)

#define PATTERN7(root, tag0, tag1, tag2, tag3, tag4, tag5, tag6) \
    root->content.nonterminal.childNum == 7 \
    && _PATTERN7(root, tag0, tag1, tag2, tag3, tag4, tag5, tag6)

#endif