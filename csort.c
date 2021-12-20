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

    if (CSortConfig_init(&csort.conf, &csort.arena, luaConfig) < 0) {
        CSort_panic(&csort, "Cannot load deafult config file: %s", luaConfig);
    }
    return csort;
}


inline void
CSort_free(CSort* csort) {
    CSort_modules_free(csort);
    CSortMemArena_free(&(csort->arena));
    CSortConfig_free(&csort->conf);
    fclose(csort->file);
}


inline void
CSort_panic(CSort* csort, const char* msg, ...) {
    fprintf(stderr, "csort: ");
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
internal inline bool
_optBool(CSort* csort, lua_State* luaCtx, const char* opt) {
    lua_getglobal(luaCtx, opt);
    CSortAssert(csort, (lua_type(luaCtx, -1) == LUA_TBOOLEAN));
    return lua_toboolean(luaCtx, -1) ? true : false;
}

internal inline u32
_optNum(CSort* csort, lua_State* luaCtx, const char* opt) {
    lua_getglobal(luaCtx, opt);
    CSortAssert(csort, (lua_type(luaCtx, -1) == LUA_TNUMBER));
    return lua_tonumber(luaCtx, -1);
}


internal inline bool
_is_nil(const CSort* csort, const char* opt) {\
    lua_State* luaCtx = csort->conf.lua;
    lua_getglobal(luaCtx, opt);
    return DEV_bool(lua_type(luaCtx, -1) == LUA_TNIL);
}

void
CSort_loadConfig(CSort* csort) {
    CSortConfig* conf = &csort->conf;
    lua_State* lua = csort->conf.lua;

    conf->squash_for_duplicate_library = _optBool(csort, lua, "squash_for_duplicate_library");
    conf->disable_wrapping = _optBool(csort, lua, "disable_wrapping");
    conf->wrap_after_n_imports = _optNum(csort, lua, "wrap_after_n_imports");
    conf->import_on_each_wrap = _optNum(csort, lua, "import_on_each_wrap");
    conf->wrap_after_col = _optNum(csort, lua, "wrap_after_col");
}


internal bool
CSort_findInKnowLibrarys(CSort* csort, const String_View library_view) {
    lua_State* luaCtx = csort->conf.lua;
    
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
CSort_nexttoken(const _ParseInfo* p) {
#define _col_offset(X) ((X) - static_buf)
    varPersist String_View tok_view = {0};
    varPersist enum CSortTokenType tok_type;
    char* tok_begin = SV_begin(p->buf_view);
    char* tok_end = SV_end(p->buf_view);
    const char* static_buf = p->buf;

    tok_begin = str_findFirstNotOf(tok_begin, tok_end, ' ');

    for (char* c = tok_begin; c != tok_end; ++c) {
        switch (*c) {
            case ' ': {
                tok_view = SV_slice(tok_begin, c);
                char* new_c = str_findFirstNotOf(c, tok_end, ' ');
                tok_type = CSort_gettokentype(&tok_view);
                if (tok_type == CSortTokenFrom) {
                    if (tok_begin == static_buf) {
                        return CSortToken_mk(tok_view, tok_type, new_c - c, p->line_counter, _col_offset(c));
                    }
                } else if (tok_type == CSortTokenImport) {
                    // TODO(emf): check if the previous token type we parsed was `from` or its at start of buffer
                    return CSortToken_mk(tok_view, tok_type, new_c - c, p->line_counter, _col_offset(c));
                } else {
                    return CSortToken_mk(tok_view, tok_type, new_c - c, p->line_counter, _col_offset(c));
                }
            }
            break;

            case ',': {
                if (c == tok_begin) {
                    tok_view = SV_buff(c, 1);
                    return CSortToken_mk(tok_view, CSortTokenComma, 0, p->line_counter, _col_offset(c));
                }
                tok_view = SV_slice(tok_begin, c);
                tok_type = CSort_gettokentype(&tok_view);
                return CSortToken_mk(tok_view, tok_type, 0, p->line_counter, _col_offset(c));
            }
            break;

            case '#':
                tok_view = SV_buff(c, 1);
                return CSortToken_mk(tok_view, CSortTokenComment, 0, p->line_counter, _col_offset(c));

            case '\n': {
                tok_view = SV_slice(tok_begin, c);
                if (c == tok_begin) {
                    return CSortToken_mk(tok_view, CSortTokenNewline, 0, p->line_counter, _col_offset(c));
                }
                tok_type = CSort_gettokentype(&tok_view);
                return CSortToken_mk(tok_view, tok_type, 0, p->line_counter, _col_offset(c));
           }
        }
    }

    return CSortToken_mk(tok_view, CSortTokenNewline, 0, p->line_counter, _col_offset(tok_begin));
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
// fills the buffer with newline and updates the `line_counter`
internal int
_getline(_ParseInfo* p) {
    bzero(p->buf, 512);
    if (! fgets(p->buf, 512, p->csort->file)) {
        return -1;
    }
    p->line_counter += 1;
    return 0;
}


internal inline const CSortToken*
_update_token(_ParseInfo* p) {
    p->prev_tok_type = p->tok->type;
    *(p->tok) = CSort_nexttoken(p);
    if (p->tok->type == CSortTokenNewline || p->tok->type == CSortTokenComment) {
        if (_getline(p) < 0) {
            *(p->tok) = CSortToken_mk(p->tok->tok_view, CSortTokenEnd, 0, 0, 0);
        } else {
            p->buf_view = SV(p->buf);
        }
    } else {
        p->buf_view = CSort_inc_buff(&p->buf_view, p->tok);
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
    varPersist const CSortToken* tok;

    CSortToken initial_tok = CSortToken_mk_initial();
    _ParseInfo parse_info = _ParseInfo_mk(csort, &initial_tok);
    if (_getline(&parse_info) < 0) {
        CSort_panic(csort, "file empty!");
    }

    while (tok = _update_token(&parse_info), tok->type != CSortTokenEnd) {
        if (tok->type == CSortTokenImport) {
            bool is_already_kept;
            CSortModuleObjNode* _import = _parse_import_statement_with_duplicate_check(csort, &parse_info, &is_already_kept);
            if (! is_already_kept) {
                CSort_append_module(csort, _import);
            }
        } else if (tok->type == CSortTokenFrom) {
            tok = _update_token(&parse_info);

            if (tok->type != CSortTokenIdentifier) {
                CSort_report_unexpected_errs(csort, tok);
                CSort_panic_tok(csort, tok, "Expected module got '%.*s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
            } else {
                bool is_already_kept = false;
                CSortModuleObjNode* _from_import;
                if (csort->conf.squash_for_duplicate_library) {
                    _from_import = _search_for_module(csort, &tok->tok_view);
                    if (! _from_import) {
                        _from_import = CSortModuleObjNode_mk(csort, &tok->tok_view, NULL, CSortModuleKind_FROM);
                        is_already_kept = false;
                    } else is_already_kept = true;
                } else {
                    _from_import = CSortModuleObjNode_mk(csort, &tok->tok_view, NULL, CSortModuleKind_FROM);
                }

                tok = _update_token(&parse_info);
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



// --------------------------------------------------------------------------------------------
//
// Print sorted imports
//
// --------------------------------------------------------------------------------------------

#define _buffer_whitespace(X, Y) fprintf((X), "%*c", (Y), ' ')
#define _buffer_newline(X, Y) (fputs("\n", (X)), _buffer_whitespace(X, Y))

void
wrap_imports(const CSortConfig* conf, CSortModuleObjNode* m, const u32* import_offset) {
    u32 count = 0;
    CSortMemArenaNode** str_node;

    fputc('(', stdout);
    for (; count < conf->wrap_after_n_imports; ++count) {
        str_node = (CSortMemArenaNode**) DynArray_get(&m->imports, count);
        fprintf(stdout, "%s, ", (char*) (*str_node)->mem);
    }

    u32 remaining = m->imports.len - count;
    assert(conf->import_on_each_wrap != 0);
    while (remaining > conf->import_on_each_wrap) {
        _buffer_newline(stdout, *import_offset);
        FOR (i, conf->import_on_each_wrap + 1) {
            str_node = (CSortMemArenaNode**) DynArray_get(&m->imports, count);
            fprintf(stdout, "%s, ", (char*) (*str_node)->mem);
        }

        remaining -= conf->import_on_each_wrap;
        count += conf->import_on_each_wrap;
    }

    // print remaining imports which weren't wrapped
    _buffer_newline(stdout, *import_offset);
    for (; count < m->imports.len - 1; ++count) {
        str_node = (CSortMemArenaNode**) DynArray_get(&m->imports, count);
        fprintf(stdout, "%s, ", (char*) (*str_node)->mem);
    }
    str_node = (CSortMemArenaNode**) DynArray_get(&m->imports, m->imports.len - 1);
    fprintf(stdout, "%s)", (char*) (*str_node)->mem);
    fputc('\n', stdout);
}


internal void
_nowrap_imports(CSortModuleObjNode* module) {
    CSortMemArenaNode** str_node;
    for (u32 i = 0; i < module->imports.len - 1; ++i) {
        str_node = (CSortMemArenaNode**) DynArray_get(&module->imports, i);
        fprintf(stdout, "%s, ", (char*) (*str_node)->mem);
    }

    str_node = (CSortMemArenaNode**) DynArray_get(&module->imports, module->imports.len - 1);
    fprintf(stdout, "%s", (char*) (*str_node)->mem);
    fputc('\n', stdout);
    return;
}


// prints processed/formatted imports
void
CSort_print_imports(const CSort* csort) {
    const CSortConfig* conf = &csort->conf;
    CSortMemArenaNode** str_node;

    for (CSortModuleObjNode* ptr = csort->modules, * module = ptr; ptr; module = ptr) {
        ptr = ptr->next;
        u32 import_offset = -3;                // Get offset little bit where the import keywords start!


        if (module->module_kind == CSortModuleKind_FROM) {
            import_offset += fprintf(stdout, "from %s import ", (char*) module->title->mem);
        } else if (module->module_kind == CSortModuleKind_IMPORT) {
            import_offset += fprintf(stdout, "import ");
        }

        _sort_imports(module);
        if (! conf->disable_wrapping) {
            if (conf->wrap_after_n_imports && module->imports.len > conf->wrap_after_n_imports) {
                wrap_imports(conf, module, &import_offset);
            } else _nowrap_imports(module);
        } else {
            _nowrap_imports(module);
        }
    }
}
