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
    u32 line_in_file;
};

extern int _compare_cstr_nodes(const CSortMemArenaNode** n1, const CSortMemArenaNode** n2);



// --------------------------------------------------------------------------------------------
typedef struct CSort CSort;
struct CSort {
    FILE* file;
    String_View fileName;

    CSortMemArena arena;
    CSortConfig conf;

    CSortModuleObjNode* modules_curr_node, * modules;
};


extern inline CSort CSort_mk(const String_View fileName, FILE* file, const char* luaConfig);
extern inline void CSort_free(CSort* csort);
extern inline void CSort_panic(CSort* csort, const char* msg, ...);
extern void CSort_loadConfig(CSort* csort);
extern void CSort_sortit(CSort* csort);
extern void CSort_do(const CSort* csort);


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
    CSortTokenString,
    CSortTokenNewline,
    CSortTokenStart,
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

internal inline CSortToken
CSortToken_mk_initial() {
    return (CSortToken) {
        .tok_view = {0},
        .type = CSortTokenStart,
        .next_tok_offset = 0,
        .line_num = 0,
        .col_offset = 0,
    };
}



// --------------------------------------------------------------------------------------------
typedef struct _ParseInfo _ParseInfo;
struct _ParseInfo {
    CSort*         csort;
    CSortToken*    tok;
    enum CSortTokenType prev_tok_type;
    char           buf[512];
    String_View    buf_view;
    u32            line_counter;
};


internal inline _ParseInfo
_ParseInfo_mk(CSort* csort, CSortToken* tok) {
    _ParseInfo p = {0};
    p.csort = csort;
    p.prev_tok_type = CSortTokenStart;
    p.tok = tok;
    p.buf_view = SV_buff(p.buf, 0);
    p.line_counter = 0;
    return p;
}

#endif
