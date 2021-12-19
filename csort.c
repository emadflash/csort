#include "csort.h"

#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>

#define CSORT_MAX(X, Y) ((X) > (Y) ? 1 : 0)
#define CSORT_MIN(X, Y) ((X) < (Y) ? 1 : 0)



// --------------------------------------------------------------------------------------------
//
// Module functions 
// 
// --------------------------------------------------------------------------------------------
internal CSortModuleObjNode*
CSortModuleObjNode_mk(CSort* csort, const String_View* title, const String_View* written_as, enum CSortModuleKind module_kind) {
    CSortModuleObjNode* module = (CSortModuleObjNode*) DEV_malloc(1, sizeof(CSortModuleObjNode));
    module->module_kind = module_kind;
    module->imports = DynArray_mk(sizeof(CSortMemArenaNode*));

    if (! title) {
        module->title = NULL;
    } else {
        module->title = CSortMemArenaCopyCStr(&csort->arena, title->data, title->len);
    }

    if (! written_as) {
        module->written_as = NULL;
    } else {
        module->written_as = CSortMemArenaCopyCStr(&csort->arena, written_as->data, written_as->len);
    }
    return module;
}

internal void
CSortModuleObjNode_free(CSort* csort, CSortModuleObjNode* n) {
    if (n->module_kind == CSortModuleKind_FROM) {
        CSortMemArena_dealloc(&csort->arena, n->title);
    }

    if (n->written_as) {
        CSortMemArena_dealloc(&csort->arena, n->title);
    }
    
    DynArray_free(&n->imports);
}

// --------------------------------------------------------------------------------------------
internal void
CSort_append_module(CSort* csort, CSortModuleObjNode* module_node) {
    if (! csort->modules_curr_node) {
        csort->modules_curr_node = module_node;
        csort->modules_curr_node->next = NULL;
        csort->modules_curr_node->prev = NULL;
        csort->modules = csort->modules_curr_node;
    } else {
        module_node->prev = csort->modules_curr_node;
        module_node->next = NULL;
        csort->modules_curr_node->next = module_node;
        csort->modules_curr_node = module_node;
    }
}


internal void
CSort_modules_free(CSort* csort) {
    for (CSortModuleObjNode* ptr = csort->modules, * tmp = ptr; ptr; tmp = ptr) {
        ptr = ptr->next;
        CSortModuleObjNode_free(csort, tmp);
        free(tmp);
    }
}


// Predicate function for comparing two CSortMemArenaNode
int
_compare_cstr_nodes(const CSortMemArenaNode** n1, const CSortMemArenaNode** n2) {
    const String_View n1_view = SV((char*) (*n1)->mem);
    const String_View n2_view = SV((char*) (*n2)->mem);
    
    char* digitBegin1 = SV_findFirstNotOfPredRev(n1_view, isdigit);
    char* digitBegin2 = SV_findFirstNotOfPredRev(n2_view, isdigit);

    digitBegin1++;
    digitBegin2++;
    if (DEV_strIsEqN(SV_data(n1_view), digitBegin1 - SV_begin(n1_view),
                SV_data(n2_view), digitBegin2 - SV_begin(n2_view))) {
        u64 intN1, intN2;
        DEV_strToInt(digitBegin1, &intN1, 10);
        DEV_strToInt(digitBegin2, &intN2, 10);

        return CSORT_MAX(intN1, intN2);
    }
    return strncmp((char*) (*n1)->mem, (char*) (*n2)->mem, (*n1)->mem_used); 
}


internal void
_sort_imports(CSortModuleObjNode* n) {
    if (n->imports.len >= 2) {
        qsort(n->imports.mem, n->imports.len, sizeof(CSortMemArenaNode*), (void*)_compare_cstr_nodes);
    }
}



// --------------------------------------------------------------------------------------------
//
// General CSort Setup
// 
// --------------------------------------------------------------------------------------------
inline CSort
CSort_mk(const String_View fileName, FILE* file, const char* luaConfig) {
    CSort csort = {0};
    csort.fileName = fileName;
    csort.file = file;
    csort.arena = CSortMemArena_mk();
    csort.modules = csort.modules_curr_node = NULL;

    if (CSortConfig_init(&csort.configManager, &csort.arena, luaConfig) < 0) {
        CSort_panic(&csort, "Cannot load deafult config file: %s", luaConfig);
    }
    return csort;
}


inline void
CSort_free(CSort* csort) {
    CSort_modules_free(csort);
    CSortMemArena_free(&(csort->arena));
    CSortConfig_free(&csort->configManager);
    fclose(csort->file);
}


inline void
CSort_panic(CSort* csort, const char* msg, ...) {
    fprintf(stderr, "panic: ");
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    putc('\n', stderr);
    CSort_free(csort);
    exit(1);
}


/**
 *
 * error with token's row, column and message
 *
 * csort:<line_number>:<column>: <error-message>
 */
internal inline void
CSort_panic_tok(CSort* csort, const CSortToken* tok, const char* msg, ...) {
    fprintf(stderr, "csort:%d:%d: ", tok->line_num, tok->col_offset);
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    putc('\n', stderr);
    CSort_free(csort);
    exit(1);
}


// These errors occur at UnExpected Newline or a comment
internal inline void
CSort_report_unexpected_errs(CSort* csort, const CSortToken* tok) {
    switch (tok->type) {
        case CSortTokenNewline:
            CSort_panic_tok(csort, tok, "UnExpected newline");
        case CSortTokenComment:
            CSort_panic_tok(csort, tok, "UnExpected comment");
    }
}



// --------------------------------------------------------------------------------------------
//
// Lua Config
// 
// --------------------------------------------------------------------------------------------
internal bool
CSort_findInKnowLibrarys(CSort* csort, const String_View library_view) {
    lua_State* luaCtx = csort->configManager.lua;
    
    lua_getglobal(luaCtx, "know_standard_library");
    if (! lua_istable(luaCtx, -1)) {
        CSort_panic(csort, "%s, Expected type LUA_TTABLE got %s ???", "know_standard_library", luaL_typename(luaCtx, -1));
    }

    
    // TODO(emf): figure out a "fast" way to save or search in know_standard_library lists
    lua_pushnil(luaCtx);
    String_View lua_str_view = {0};
    while (lua_next(luaCtx, -2) != 0) {
        CSortAssert(csort, (lua_type(luaCtx, -1) == LUA_TSTRING));
        lua_str_view = SV(lua_tostring(luaCtx, -1));
        if (SV_isEq(library_view, lua_str_view)) {
            return true;
        }

        lua_pop(luaCtx, 1);
    }

    return false;
}


internal bool
CSortConfig_booleanOption(CSort* csort, const char* opt) {
    lua_State* luaCtx = csort->configManager.lua;
    lua_getglobal(luaCtx, opt);
    CSortAssert(csort, (lua_type(luaCtx, -1) == LUA_TBOOLEAN));
    return lua_toboolean(luaCtx, -1) ? true : false;
}



// --------------------------------------------------------------------------------------------
//
// Tokenizer
// 
// --------------------------------------------------------------------------------------------
internal inline enum CSortTokenType
CSort_gettokentype(const String_View* tok_view) {
    if (SV_isEq(*tok_view, SV("from"))) {
        return CSortTokenFrom;
    } else if (SV_isEq(*tok_view, SV("import"))) {
        return CSortTokenImport;
    }
    return CSortTokenIdentifier;
}


internal inline String_View
CSort_inc_buff(const String_View* prev_buff, const CSortToken* tok) {
    return SV_buff(SV_end(tok->tok_view) + tok->next_tok_offset, prev_buff->len - SV_len(tok->tok_view) - tok->next_tok_offset);
}


internal CSortToken
CSort_nexttoken(CSort* csort, const char* static_buf_begin, const String_View* buf_view, const u32* line_counter) {
#define _col_offset(X) ((X) - static_buf_begin)
    varPersist String_View tok_view = {0};
    varPersist enum CSortTokenType tok_type;
    char* tok_begin = SV_begin(*buf_view);
    char* tok_end = SV_end(*buf_view);

    tok_begin = str_findFirstNotOf(tok_begin, tok_end, ' ');

    for (char* c = tok_begin; c != tok_end; ++c) {
        switch (*c) {
            case ' ': {
                tok_view = SV_slice(tok_begin, c);
                char* new_c = str_findFirstNotOf(c, tok_end, ' ');
                tok_type = CSort_gettokentype(&tok_view);
                return CSortToken_mk(tok_view, tok_type, new_c - c, *line_counter, _col_offset(c));
            }
            break;

            case ',': {
                if (c == tok_begin) {
                    tok_view = SV_buff(c, 1);
                    return CSortToken_mk(tok_view, CSortTokenComma, 0, *line_counter, _col_offset(c));
                }
                tok_view = SV_slice(tok_begin, c);
                tok_type = CSort_gettokentype(&tok_view);
                return CSortToken_mk(tok_view, tok_type, 0, *line_counter, _col_offset(c));
            }
            break;

            case '#':
                tok_view = SV_buff(c, 1);
                return CSortToken_mk(tok_view, CSortTokenComment, 0, *line_counter, _col_offset(c));

            case '\n': {
                tok_view = SV_slice(tok_begin, c);
                if (c == tok_begin) {
                    return CSortToken_mk(tok_view, CSortTokenNewline, 0, *line_counter, _col_offset(c));
                }
                tok_type = CSort_gettokentype(&tok_view);
                return CSortToken_mk(tok_view, tok_type, 0, *line_counter, _col_offset(c));
           }
        }
    }

    return CSortToken_mk(tok_view, CSortTokenNewline, 0, *line_counter, _col_offset(tok_begin));
}



// fills the buffer with newline and updates the `line_counter`
internal int
_getline(const CSort* csort, char* buf, u32 buf_size, u32* line_counter) {
    if (! fgets(buf, buf_size, csort->file)) {
        return -1;
    }
    ++*line_counter;
    return 0;
}


internal CSortModuleObjNode*
_search_for_module(const CSort* csort, const String_View* tok_view) {
    for (CSortModuleObjNode* ptr = csort->modules; ptr; ptr = ptr->next) {
        if (ptr->module_kind == CSortModuleKind_FROM) {
            if (SV_isEqRaw(*tok_view, (char*) ptr->title->mem)) {
                return ptr;
            }
        }
    }

    return NULL;
}


internal CSortModuleObjNode*
_search_for_imports(const CSort* csort, const String_View* tok_view) {
    for (CSortModuleObjNode* ptr = csort->modules; ptr; ptr = ptr->next) {
        if (ptr->module_kind == CSortModuleKind_IMPORT) {
            for (u32 i = 0; i < ptr->imports.len; ++i) {
                CSortMemArenaNode** str_node = (CSortMemArenaNode**) DynArray_get(&ptr->imports, i);
                if (SV_isEqRaw(*tok_view, (char*) (*str_node)->mem)) {
                    return ptr;
                }
            }
        }
    }

    return NULL;
}


internal bool
_search_import_from_statement(CSortModuleObjNode* n, const String_View* tok_view) {
    CSortMemArenaNode** str_node;
    for (u32 i = 0; i < n->imports.len; ++i) {
        str_node = (CSortMemArenaNode**) DynArray_get(&n->imports, i);
        if (SV_isEqRaw(*tok_view, (char*) (*str_node)->mem)) {
            return true;
        }
    }

    return false;
}



// --------------------------------------------------------------------------------------------
//
// Parse functions
// 
// --------------------------------------------------------------------------------------------
typedef struct _ParseInfo _ParseInfo;
struct _ParseInfo {
    CSort*       csort;
    CSortToken*  tok;
    char*        static_buf_begin;                      // Save buf_view begin for getting offset
    String_View* buf_view;
    u32*         line_counter;
};


internal inline _ParseInfo
_ParseInfo_mk(CSort* csort, CSortToken* tok, const char* static_buf_begin, String_View* buf_view, u32* line_counter) {
    return (_ParseInfo) {
        .csort = csort,
        .tok = tok,
        .static_buf_begin = static_buf_begin,
        .buf_view = buf_view,
        .line_counter = line_counter,
    };
}


internal inline const CSortToken*
_update_token(_ParseInfo* p) {
    *(p->tok) = CSort_nexttoken(p->csort, p->static_buf_begin, p->buf_view, p->line_counter);
    if (p->tok->type == CSortTokenNewline || p->tok->type == CSortTokenComment) {
        if (_getline(p->csort, p->static_buf_begin, 512, p->line_counter) < 0) {
            *(p->tok) = CSortToken_mk(p->tok->tok_view, CSortTokenEnd, 0, 0, 0);
        } else {
            *(p->buf_view) = SV(p->buf_view->data);
            p->static_buf_begin = p->buf_view->data;
        }
    } else {
        *(p->buf_view) = CSort_inc_buff(p->buf_view, p->tok);
    }

    return p->tok;
}


internal void
_push_import(CSort* csort, CSortModuleObjNode* n, const String_View* tok_view) {
    CSortMemArenaNode* s = CSortMemArenaCopyCStr(&csort->arena, tok_view->data, tok_view->len);
    DynArray_push(&n->imports, (void*)&s);
}


// parses import statement with duplicate check
internal CSortModuleObjNode*
_parse_import_statement_with_duplicate_check(CSort* csort, _ParseInfo* parse_info, bool* is_already_kept) {
    const CSortToken* tok;;
    CSortModuleObjNode* _import = NULL;
    *is_already_kept = true;

    do {
        tok =_update_token(parse_info);
        if (tok->type != CSortTokenIdentifier) {
            CSort_report_unexpected_errs(csort, tok);
            CSort_panic_tok(csort, tok, "Expected module got '%*.s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
        }

        _import = _search_for_imports(csort, &tok->tok_view);
        if (! _import) {
            *is_already_kept = false;
            _import = CSortModuleObjNode_mk(csort, NULL, NULL, CSortModuleKind_IMPORT);
            _push_import(csort, _import, &tok->tok_view);

            tok =_update_token(parse_info);
            if (tok->type == CSortTokenComma) {
                goto _push_more_imports;
            } else goto _return;
        }
    } while (tok = _update_token(parse_info), tok->type == CSortTokenComma || tok->type == CSortTokenIdentifier);
        
_return:
    return _import;

_push_more_imports:
    do {
        tok = _update_token(parse_info);
        if (tok->type != CSortTokenIdentifier) {
            CSort_panic_tok(csort, tok, "Expected module got '%*.s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
        }

        _push_import(csort, _import, &tok->tok_view);
        tok = _update_token(parse_info);
    } while (tok->type == CSortTokenComma || tok->type == CSortTokenIdentifier);

    return _import;
}


// parses import statement
internal void
_parse_import_after_from(CSort* csort, _ParseInfo* parse_info, CSortModuleObjNode* n) {
    const CSortToken* tok;
    do {
        tok = _update_token(parse_info);
        if (tok->type != CSortTokenIdentifier) {
            CSort_report_unexpected_errs(csort, tok);
            CSort_panic_tok(csort, tok, "Expected module got '%*.s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
        }

        if (! _search_import_from_statement(n, &tok->tok_view)) {
            _push_import(csort, n, &tok->tok_view);
        }
        tok = _update_token(parse_info);
    } while (tok->type == CSortTokenComma || tok->type == CSortTokenIdentifier);
}



// --------------------------------------------------------------------------------------------
void
CSort_sortit(CSort* csort) {
    varPersist CSortToken initial_tok = {0};
    varPersist const CSortToken* tok;
    varPersist char buf[512];
    u32 line_counter = 0;

    if (_getline(csort, buf, 512, &line_counter) < 0) {
        CSort_panic(csort, "file is empty");
    }

    String_View buf_view = SV(buf);
    _ParseInfo parse_info = _ParseInfo_mk(csort, &initial_tok, SV_begin(buf_view), &buf_view, &line_counter);

    while (tok = _update_token(&parse_info), tok->type != CSortTokenEnd) {
        if (tok->type == CSortTokenImport) {
            bool is_already_kept;
            CSortModuleObjNode* _import = _parse_import_statement_with_duplicate_check(csort, &parse_info, &is_already_kept);
            if (! is_already_kept) {
                CSort_append_module(csort, _import);
            }
        } else if (tok->type == CSortTokenFrom) {
            _update_token(&parse_info);

            if (tok->type != CSortTokenIdentifier) {
                CSort_report_unexpected_errs(csort, tok);
                CSort_panic_tok(csort, tok, "Expected module got '%.*s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
            } else {
                bool is_already_kept = false;
                CSortModuleObjNode* _from_import;
                if (CSortConfig_booleanOption(csort, "squash_for_duplicate_library")) {
                    _from_import = _search_for_module(csort, &tok->tok_view);
                    if (! _from_import) {
                        _from_import = CSortModuleObjNode_mk(csort, &tok->tok_view, NULL, CSortModuleKind_FROM);
                        is_already_kept = false;
                    } else is_already_kept = true;
                } else {
                    _from_import = CSortModuleObjNode_mk(csort, &tok->tok_view, NULL, CSortModuleKind_FROM);
                }

                _update_token(&parse_info);
                if (tok->type != CSortTokenImport) {
                    CSort_panic_tok(csort, tok, "Expected import statement got '%.*s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
                }

                _parse_import_after_from(csort, &parse_info, _from_import);
                if (! is_already_kept) {
                    CSort_append_module(csort, _from_import);
                }
            }
        }
    }
}


// prints processed/formatted imports
void
CSort_print_imports(const CSort* csort) {
    CSortMemArenaNode** str_node;
    for (CSortModuleObjNode* ptr = csort->modules, * module = ptr; ptr; module = ptr) {
        ptr = ptr->next;
        if (module->module_kind == CSortModuleKind_FROM) {
            fprintf(stdout, "from %s import ", (char*) module->title->mem);
        } else if (module->module_kind == CSortModuleKind_IMPORT) {
            fprintf(stdout, "import ");
        }

        _sort_imports(module);
        for (u32 i = 0; i < module->imports.len - 1; ++i) {
            str_node = (CSortMemArenaNode**) DynArray_get(&module->imports, i);
            fprintf(stdout, "%s, ", (char*) (*str_node)->mem);
        }

        str_node = (CSortMemArenaNode**) DynArray_get(&module->imports, module->imports.len - 1);
        fprintf(stdout, "%s", (char*) (*str_node)->mem);
        fputc('\n', stdout);
    }
}
