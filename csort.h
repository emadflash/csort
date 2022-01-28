#ifndef __CSORT_H__
#define __CSORT_H__

#include "core.h"
#include "config.h"

#include "external/lua/lua.h"
#include "external/lua/lualib.h"
#include "external/lua/lauxlib.h"

#include <stdio.h>
#include <errno.h>
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

void CSortOptParse_show_usage(FILE* output_stream, const CSortOptObj* options, u32 options_len);
void CSortOptParse(int argc, char* argv[], CSortOptObj* options, u32 options_len, const char* prepend_error_msg);



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

int _compare_cstr_nodes(const CSortMemArenaNode** n1, const CSortMemArenaNode** n2);



// --------------------------------------------------------------------------------------------
typedef struct CSort CSort;
struct CSort {
    CSortMemArena arena;
    CSortConfig conf;
};

CSort CSort_mk();
extern inline void CSort_init_config(CSort* csort, const char* lua_config);
extern inline void CSort_deinit(CSort* csort);
extern inline void CSort_panic(CSort* csort, const char* msg, ...);
extern void CSort_load_config(CSort* csort);


// --------------------------------------------------------------------------------------------
typedef struct CSortEntity CSortEntity;
struct CSortEntity {
    CSort* csort;
    FILE* input_file;
    const char* file_to_sort;

    CSortModuleObjNode* modules_curr_node, * modules;
};

extern inline CSortEntity CSortEntity_mk(CSort* csort, const char* file_to_sort);
extern void CSortEntity_do(CSortEntity* entity);
extern void CSortEntity_free(CSortEntity* entity);
extern void CSortEntity_deinit(CSortEntity* entity);


// --------------------------------------------------------------------------------------------
int CSortGetExtension(const String_View sv, String_View* ext);
String CSortAppendPath(const char* path, const char* add);
void CSortPerformOnFileCallback(CSort* csort, char* input_path, void (callback)(CSort* csort, const char* file_path, const char* file_name));

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
    CSortTokenIdentifier,
    CSortTokenString,
    CSortTokenNewline,
    CSortTokenComment,
    CSortTokenEof,
};

// Start of symbols, which are useful for us
// from, import, as
#define SYMBOLS_START\
    case 'f':\
    case 'i':\
    case 'a':

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

// --------------------------------------------------------------------------------------------
//
// Tokenzier
//
// --------------------------------------------------------------------------------------------
typedef struct ParseCtx ParseCtx;
struct ParseCtx {
    CSort* csort;
    CSortMemArenaNode* data;
    char* curr, *next, *end;
    int ch;
    u32 line_counter;
};

internal CSortMemArenaNode*
readfile_into(CSortMemArena* arena, FILE* fp) {
    CSortMemArenaNode* m = CSortMemArena_alloc(arena);
    char buf[512];
    u32 buf_len;
    while (fgets(buf, 512, fp)) {
        buf_len = strlen(buf);
        CSortMemArenaNode_fill(m, buf, buf_len);
        DEV_memzero(buf, 512);
    }
    CSortMemArenaNode_fill(m, (void*) "\0", 1);
    return m;
}

internal inline ParseCtx
ParseCtx_mk(CSort* csort, const CSortEntity* entity) {
    ParseCtx p = {0};
    p.csort = csort;
    p.data = readfile_into(&csort->arena, entity->input_file);
    // Set #curr, #next and #end
    p.curr = p.next = (char*) p.data->mem;
    p.next += 1;
    p.end = (char*) p.data->mem + p.data->mem_used;
    p.ch = (p.curr != p.end) ? *p.curr : -1;
    p.line_counter = 0;
    return p;
}

internal void
ParseCtx_deinit(ParseCtx* ctx) {
    CSortMemArena_dealloc(&ctx->csort->arena, ctx->data);
}

CSortToken ParseCtxNextToken(ParseCtx* ctx);

#endif
