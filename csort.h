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
//
// CSortOpt
//
// --------------------------------------------------------------------------------------------
typedef struct CSortOptObj CSortOptObj;
struct CSortOptObj {
    char* short_flag, *long_flag, *about;
    char* Str;
    u64* Int;
    bool* Bool;
};


#define CSortOptParse_is_help_flag(arg) (DEV_strIsEq(arg, "--help") || DEV_strIsEq(arg, "-h"))
#define declare_CSortOpt()\
extern CSortOptObj CSortOptInt(CSort* csort, u64* data_ptr, const char* long_flag, const char* short_flag, const char* about);\
extern CSortOptObj CSortOptBool(CSort* csort, bool* data_ptr, const char* long_flag, const char* short_flag, const char* about);\
extern CSortOptObj CSortOptStr(CSort* csort, char* data_ptr, const char* long_flag, const char* short_flag, const char* about)\

#define typedef_CSortOpt(dataType, T)                                                                            \
CSortOptObj                                                                                                      \
CSortOpt##T(CSort* csort, dataType* data_ptr, const char* long_flag, const char* short_flag, const char* about) {\
    CSortOptObj opt = {0};                                                                                       \
    opt.long_flag = long_flag;                                                                                   \
    opt.short_flag = short_flag;                                                                                 \
    opt.about = about;                                                                                           \
    opt.Int = NULL;                                                                                              \
    opt.Str = NULL;                                                                                              \
    opt.Bool = NULL;                                                                                             \
    opt.T = data_ptr;                                                                                            \
    return opt;                                                                                                  \
}

extern void CSortOptParse_show_usage(FILE* output_stream, const CSortOptObj* options, u32 options_len);
extern void CSortOptParse(int argc, char* argv[], CSortOptObj* options, u32 options_len, const char* prepend_error_msg);



// --------------------------------------------------------------------------------------------
//
// Module struct, Tokenzier
//
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
    FILE* input_file;
    const char* file_to_sort;

    CSortMemArena arena;
    CSortConfig conf;

    CSortModuleObjNode* modules_curr_node, * modules;
};


extern CSort CSort_mk();
extern inline void CSort_init_target(CSort* csort, const char* file_to_sort);
extern inline void CSort_init_config(CSort* csort, const char* lua_config);
extern inline void CSort_free(CSort* csort);
extern inline void CSort_panic(CSort* csort, const char* msg, ...);
extern void CSort_load_config(CSort* csort);
extern void CSort_sortit(CSort* csort);
extern void CSort_do(const CSort* csort);


// Declare CSortOpt functions defined by @macro(typedef_CSortOpt)
declare_CSortOpt();


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
