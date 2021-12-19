#ifndef __CSORT_H__
#define __CSORT_H__

#include "core.h"
#include "config.h"

#include "external/lua/include/lua.h"
#include "external/lua/include/lualib.h"
#include "external/lua/include/lauxlib.h"

#include <stdio.h>
#include <assert.h>



// --------------------------------------------------------------------------------------------
enum CSortModuleKind {
    CSortModuleKind_FROM,
    CSortModuleKind_IMPORT,
    CSortModuleKind_IMPORT_AS,
};


// --------------------------------------------------------------------------------------------
typedef struct CSortModuleObjNode CSortModuleObjNode;
struct CSortModuleObjNode {
    CSortMemArenaNode*   title;                         // In case of import via from
    CSortMemArenaNode*   written_as;

    DynArray             imports;
    enum CSortModuleKind module_kind;
    
    CSortModuleObjNode*  prev;
    CSortModuleObjNode*  next;
};

extern int _compare_cstr_nodes(const CSortMemArenaNode** n1, const CSortMemArenaNode** n2);



// --------------------------------------------------------------------------------------------
typedef struct CSort CSort;
struct CSort {
    FILE* file;
    String_View fileName;

    CSortMemArena arena;
    CSortConfig configManager;

    CSortModuleObjNode* modules_curr_node, * modules;
};

extern inline CSort CSort_mk(const String_View fileName, FILE* file, const char* luaConfig);
extern inline void CSort_free(CSort* csort);
extern inline void CSort_panic(CSort* csort, const char* msg, ...);
extern void CSort_sortit(CSort* csort);


#define CSortAssert(X, Y)                 \
    if (! Y) {                            \
        CSort_panic(X, "assertion error");\
    }



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
    u32 line_num;
    u32 col_offset;
};

internal inline CSortToken
CSortToken_mk(const String_View tok_view, enum CSortTokenType type, u32 next_tok_offset, u32 line_num, u32 col_offset) {
    return (CSortToken) {
        .tok_view = tok_view,
        .type = type,
        .next_tok_offset = next_tok_offset,
        .line_num = line_num,
        .col_offset = col_offset,
    };
}

#endif
