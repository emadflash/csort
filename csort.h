#ifndef __CSORT_H__
#define __CSORT_H__

#include <stdio.h>
#include "core.h"


typedef struct CSort CSort;
struct CSort {
    FILE* file;
    String_View fileName;

    CSortMemArena arena;
};

extern inline CSort CSort_mk(const String_View fileName, FILE* file);
extern inline void CSort_free(CSort* csort);
extern inline void CSort_panic(CSort* csort, const char* msg);
extern void CSort_sortit(CSort* csort);



// --------------------------------------------------------------------------------------------
enum CSortTokenType {
    CSortTokenFrom,
    CSortTokenImport,
    CSortTokenComma,
    CSortTokenSpace,
    CSortTokenIdentifier,
    CSortTokenNewline,
    CSortTokenEnd,
    CSortTokenComment,
};

typedef struct CSortToken CSortToken;
struct CSortToken {
    String_View tok_view;
    enum CSortTokenType type;

    u32 next_tok_offset;
};

internal inline CSortToken
CSortToken_mk(const String_View tok_view, enum CSortTokenType type, u32 next_tok_offset) {
    return (CSortToken) {
        .tok_view = tok_view,
        .type = type,
        .next_tok_offset = next_tok_offset,
    };
}

#endif
