#include "csort.h"

#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _DIRENT_HAVE_D_TYPE
#error "Requires _DIRENT_HAVE_D_TYPE, to check if #dirent is a directory."
#endif

#define CSORT_MAX(X, Y) ((X) > (Y) ? 1 : 0)
#define CSORT_MIN(X, Y) ((X) < (Y) ? 1 : 0)

void log_error(char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fputs("error: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

// --------------------------------------------------------------------------------------------
//
// CSortOpt
//
// --------------------------------------------------------------------------------------------
typedef_CSortOpt(u64, Int)
typedef_CSortOpt(char, Str)
typedef_CSortOpt(bool, Bool)

internal CSortOptObj*
CSortOptObj_from_options(CSortOptObj* options, u32 options_len, const char* arg) {
    FOR (i, options_len) {
        if (DEV_strIsEq(options[i].short_flag, arg) || DEV_strIsEq(options[i].long_flag, arg)) {
            return &options[i]; 
        }
    }
    return NULL;
}


// prints description about options in @param(output_stream)
void
CSortOptParse_show_usage(FILE* output_stream, const CSortOptObj* options, u32 options_len) {
#define GetType(X) (((X)->Int) ? "Int" : (((X)->Str) ? "Str" : (((X)->Bool) ? "Bool" : "NONE")))
    fprintf(stderr, "options: \n\n");

    FOR (i, options_len) {
        const CSortOptObj* obj = &options[i];
        fprintf(output_stream, "  %s| %s: [%s]\n", obj->short_flag, obj->long_flag, GetType(obj));
        fprintf(output_stream, "    %s\n\n", obj->about);
    }
}


void
CSortOptParse(int argc, char* argv[], CSortOptObj* options, u32 options_len, const char* prepend_error_msg) {
#define error(...)        \
    eprintln(__VA_ARGS__);\
    exit(1);

    u32 arg_counter = 0;
    char* arg = argv[arg_counter];
    CSortOptObj* obj;

    while (arg = argv[arg_counter++], arg) {
        if (CSortOptParse_is_help_flag(arg)) {
            eprintln(prepend_error_msg);
            CSortOptParse_show_usage(stderr, options, options_len);
            exit(1);
        }

        if (obj = CSortOptObj_from_options(options, options_len, arg), obj) {
            if (obj->Int) {
                arg = argv[arg_counter++];
                if (arg) {
                    if (DEV_strToInt(arg, obj->Int, 10) < 0) {
                        error("error: '%s': Expected a integer, got %s", obj->long_flag, arg);
                    }
                } else error("error: '%s': Expected a integer, got newline", obj->long_flag);
            }

            if (obj->Str) {
                arg = argv[arg_counter++];
                if (arg) {
                    obj->Str = arg;
                } else error("error: '%s': Expected a string, got newline", obj->long_flag);
            }

            if (obj->Bool) {
                *obj->Bool = (*obj->Bool) ? false : true;
            }
        } else {
            error("error: '%s': UnKnown thing", arg);
        }
    }
}


// --------------------------------------------------------------------------------------------
//
// Directory listing
//
// --------------------------------------------------------------------------------------------
int
CSortGetExtension(const String_View sv, String_View* ext) {
    char* e = SV_rfind(sv, '.');
    // Discard #sv if it begins with a '.' (dot)
    if (e && e != SV_begin(sv)) {
        *ext = SV_slice(e, SV_end(sv));
        return 0;
    }
    return -1;
}

// get endpoint aka filename from #path
// example: ./path/filename -> filename
char*
get_dir_endpoint(char* path, u32 len) {
    char* e = path + len - 1;
    for (; e != path - 1 &&  *e != '/'; --e) {
    }
    e += 1;
    return e;
}

// Append #to_add to #path and result is stored on the stack, I hope.
int
append_path(const char* path, const char* to_add, char* newpath, u32 len) {
    const int newlen = strlen(path) + strlen(to_add) + 2;
    if (len < newlen) {
        return -1;
    }
    return snprintf(newpath, newlen, "%s/%s", path, to_add);
}

int
CSortPerformOnFileCallback(CSort* csort, const char* input_path, void (callback)(CSort* csort, const char* file_path)) {
    DIR* dirp = opendir(input_path);
    if (! dirp) {
        log_error("opendir: Could not open: %s: %s", input_path, strerror(errno));
        closedir(dirp);
        return -1;
    }

    struct dirent* d;
    while ((d = readdir(dirp))) {
        if (! DEV_strIsEq(d->d_name, "..") && ! DEV_strIsEq(d->d_name, ".")) {
            if (d->d_type == DT_REG) {
                char newpath[1024];
                const int newpath_len =  append_path(input_path, d->d_name, newpath, 1024);
                if (newpath_len < 0) {
                    log_error("#newpath len exceeded the max len. Skipping file...: %s/%s", input_path, d->d_name);
                    continue;
                }
                callback(csort, newpath);
            }
        }
    }
    closedir(dirp);
    return 0;
}

int
CSortPerformOnFileCallbackRecur(CSort* csort, const char* input_path, void (callback)(CSort* csort, const char* input_filepath)) {
    DIR* dirp = opendir(input_path);
    if (! dirp) {
        log_error("opendir: Could not open: %s: %s", input_path, strerror(errno));
        closedir(dirp);
        return -1;
    }

    struct dirent* d;
    while ((d = readdir(dirp))) {
        if (! DEV_strIsEq(d->d_name, "..") && ! DEV_strIsEq(d->d_name, ".")) {
            char newpath[1024];
            int newpath_len =  append_path(input_path, d->d_name, newpath, 1024);
            if (newpath_len < 0) {
                log_error("#newpath len exceeded the max len. Skipping file...: %s/%s", input_path, d->d_name);
                continue;
            }

            if (d->d_type == DT_DIR) {
                if (! CSortConfigFindStrList(&csort->conf, 1, get_dir_endpoint(newpath, newpath_len))) {
                    CSortPerformOnFileCallbackRecur(csort, newpath, callback);
                }
            } else {
                if (d->d_type == DT_REG) {
                    callback(csort, newpath);
                }
            }
        }
    }
    closedir(dirp);
    return 0;
}

// --------------------------------------------------------------------------------------------
//
// Module functions 
// 
// --------------------------------------------------------------------------------------------
internal CSortModuleObjNode*
CSortModuleObjNode_mk(CSortEntity* entity, const String_View* title, const String_View* written_as, enum CSortModuleKind module_kind) {
    CSortModuleObjNode* module = (CSortModuleObjNode*) DEV_malloc(1, sizeof(CSortModuleObjNode));
    module->module_kind = module_kind;
    module->imports = DynArray_mk(sizeof(CSortMemArenaNode*));

    if (! title) {
        module->title = NULL;
    } else {
        module->title = CSortMemArenaCopyCStr(&entity->csort->arena, title->data, title->len);
    }

    if (! written_as) {
        module->written_as = NULL;
    } else {
        module->written_as = CSortMemArenaCopyCStr(&entity->csort->arena, written_as->data, written_as->len);
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
CSortEntity_append_module(CSortEntity* entity, CSortModuleObjNode* module_node, u32 line_in_file) {
    module_node->line_in_file = line_in_file;
    if (! entity->modules_curr_node) {
        entity->modules_curr_node = module_node;
        entity->modules_curr_node->next = NULL;
        entity->modules_curr_node->prev = NULL;
        entity->modules = entity->modules_curr_node;
    } else {
        module_node->prev = entity->modules_curr_node;
        module_node->next = NULL;
        entity->modules_curr_node->next = module_node;
        entity->modules_curr_node = module_node;
    }
}


void
CSortEntity_deinit(CSortEntity* entity) {
    for (CSortModuleObjNode* ptr = entity->modules, * tmp = ptr; ptr; tmp = ptr) {
        ptr = ptr->next;
        CSortModuleObjNode_free(entity->csort, tmp);
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
CSort
CSort_mk() {
    CSort csort = {0};
    csort.arena = CSortMemArena_mk();
    return csort;
}

inline CSortEntity
CSortEntity_mk(CSort* csort, const char* file_to_sort) {
    CSortEntity entity = {0};
    entity.csort = csort;
    entity.file_to_sort = file_to_sort;
    entity.input_file = DEV_fopen(file_to_sort, "r");
    return entity;
}


inline void
CSort_init_config(CSort* csort, const char* lua_config) {
    if (! lua_config) {
        CSortConfig_init(&csort->conf, &csort->arena);
    } else {
        if (CSortConfig_init_w_lua(&csort->conf, &csort->arena, lua_config) < 0) {
            // Could possible be a double-free here
            CSort_panic(csort, "error: could not open: %s", lua_config);
        }
        CSort_load_config(csort);
    }
}


inline void
CSort_deinit(CSort* csort) {
    CSortMemArena_free(&(csort->arena));
    CSortConfig_deinit(&csort->conf);
}


inline void
CSort_panic(CSort* csort, const char* msg, ...) {
    fprintf(stderr, "csort: ");
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    putc('\n', stderr);
    CSort_deinit(csort);
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
    CSort_deinit(csort);
    exit(1);
}


// These errors occur at UnExpected Newline or a comment
internal inline void
CSortEntity_report_unexpected_errs(CSortEntity* entity, const CSortToken* tok, const char* expected_token_type) {
#define _unexpected_error(X, ...)\
    case (X):\
        CSort_panic_tok(entity->csort, tok, __VA_ARGS__)

    switch (tok->type) {
        _unexpected_error(CSortTokenNewline, "Expected %s, got newline", expected_token_type);
        _unexpected_error(CSortTokenComment, "Expected %s, got comment", expected_token_type);
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
CSort_load_config(CSort* csort) {
    CSortConfig* conf = &csort->conf;
    lua_State* lua = csort->conf.lua;

    // load strings in know_standard_library into memory
    if (array_push_from_str(&conf->know_standard_library, lua, "know_standard_library") < 0) {
        CSort_panic(csort, "%s, Expected type LUA_TTABLE got %s ???", "know_standard_library", luaL_typename(lua, -1));
    }
    qsort(conf->know_standard_library.mem, conf->know_standard_library.len, sizeof(String), string_strncmp);

    // load strings in skip_directories into memory
    if (array_push_from_str(&conf->skip_directories, lua, "skip_directories") < 0) {
        CSort_panic(csort, "%s, Expected type LUA_TTABLE got %s ???", "skip_directories", luaL_typename(lua, -1));
    }
    qsort(conf->skip_directories.mem, conf->skip_directories.len, sizeof(String), string_strncmp);

    // load strings in #file_exts into memory
    if (array_push_from_str(&conf->file_exts, lua, "file_exts") < 0) {
        CSort_panic(csort, "%s, Expected type LUA_TTABLE got %s ???", "skip_directories", luaL_typename(lua, -1));
    }
    qsort(conf->file_exts.mem, conf->file_exts.len, sizeof(String), string_strncmp);

    conf->squash_for_duplicate_library = _optBool(csort, lua, "squash_for_duplicate_library");
    conf->disable_wrapping = _optBool(csort, lua, "disable_wrapping");
    conf->wrap_after_n_imports = _optNum(csort, lua, "wrap_after_n_imports");
    conf->import_on_each_wrap = _optNum(csort, lua, "import_on_each_wrap");
    conf->wrap_after_col = _optNum(csort, lua, "wrap_after_col");
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
CSortEntity_find_module(const CSortEntity* entity, const String_View* tok_view) {
    for (CSortModuleObjNode* ptr = entity->modules; ptr; ptr = ptr->next) {
        if (ptr->module_kind == CSortModuleKind_FROM) {
            if (SV_isEqRaw(*tok_view, (char*) ptr->title->mem)) {
                return ptr;
            }
        }
    }
    return NULL;
}


internal CSortModuleObjNode*
_search_for_imports(const CSortEntity* entity, const String_View* tok_view) {
    for (CSortModuleObjNode* ptr = entity->modules; ptr; ptr = ptr->next) {
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
    if (! fgets(p->buf, 512, p->entity->input_file)) {
        return -1;
    }
    p->line_counter += 1;
    return 0;
}


internal inline const CSortToken*
_update_token(_ParseInfo* p) {
    *(p->tok) = CSort_nexttoken(p);
    if (p->tok->type == CSortTokenNewline || p->tok->type == CSortTokenComment || p->tok->type == CSortTokenStart) {
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
_push_import(CSortEntity* entity, CSortModuleObjNode* n, const String_View* tok_view) {
    CSortMemArenaNode* s = CSortMemArenaCopyCStr(&entity->csort->arena, tok_view->data, tok_view->len);
    DynArray_push(&n->imports, (void*)&s);
}


// parses import statement with with check for duplicate modules
internal CSortModuleObjNode*
_parse_import_statement_with_duplicate_check(CSortEntity* entity, _ParseInfo* parse_info, bool* is_already_kept) {
    const CSortToken* tok;;
    CSortModuleObjNode* _import = NULL;
    *is_already_kept = true;

    do {
        tok =_update_token(parse_info);
        if (tok->type != CSortTokenIdentifier) {
            CSortEntity_report_unexpected_errs(entity, tok, "module");
            CSort_panic_tok(entity->csort, tok, "Expected module got '%*.s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
        }

        _import = _search_for_imports(entity, &tok->tok_view);
        if (! _import) {
            *is_already_kept = false;
            _import = CSortModuleObjNode_mk(entity, NULL, NULL, CSortModuleKind_IMPORT);
            _push_import(entity, _import, &tok->tok_view);

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
            CSort_panic_tok(entity->csort, tok, "Expected module got '%*.s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
        }

        _push_import(entity, _import, &tok->tok_view);
        tok = _update_token(parse_info);
    } while (tok->type == CSortTokenComma || tok->type == CSortTokenIdentifier);

    return _import;
}


// parses import statement
internal void
_parse_import_after_from(CSortEntity* entity, _ParseInfo* parse_info, CSortModuleObjNode* n) {
    const CSortToken* tok;
    do {
        tok = _update_token(parse_info);
        if (tok->type != CSortTokenIdentifier) {
            CSortEntity_report_unexpected_errs(entity, tok, "module");
            CSort_panic_tok(entity->csort, tok, "Expected module got '%*.s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
        }

        if (! _search_import_from_statement(n, &tok->tok_view)) {
            _push_import(entity, n, &tok->tok_view);
        }
        tok = _update_token(parse_info);
    } while (tok->type == CSortTokenComma || tok->type == CSortTokenIdentifier);
}



// --------------------------------------------------------------------------------------------
void
CSortEntity_sort(CSortEntity* entity) {
    varPersist const CSortToken* tok;

    CSort* csort = entity->csort;
    CSortToken initial_tok = CSortToken_mk_initial();
    _ParseInfo parse_info = _ParseInfo_mk(entity, &initial_tok);

    while (tok = _update_token(&parse_info), tok->type != CSortTokenEnd) {
        if (tok->type == CSortTokenImport) {
            bool is_already_kept;
            CSortModuleObjNode* _import = _parse_import_statement_with_duplicate_check(entity, &parse_info, &is_already_kept);
            if (! is_already_kept) {
                CSortEntity_append_module(entity, _import, parse_info.line_counter);
            }
        } else if (tok->type == CSortTokenFrom) {
            tok = _update_token(&parse_info);

            if (tok->type != CSortTokenIdentifier) {
                CSortEntity_report_unexpected_errs(entity, tok, "module");
                CSort_panic_tok(entity->csort, tok, "Expected module got '%.*s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
            } else {
                bool is_already_kept = false;
                CSortModuleObjNode* _from_import;
                if (csort->conf.squash_for_duplicate_library) {
                    _from_import = CSortEntity_find_module(entity, &tok->tok_view);
                    if (! _from_import) {
                        _from_import = CSortModuleObjNode_mk(entity, &tok->tok_view, NULL, CSortModuleKind_FROM);
                        is_already_kept = false;
                    } else is_already_kept = true;
                } else {
                    _from_import = CSortModuleObjNode_mk(entity, &tok->tok_view, NULL, CSortModuleKind_FROM);
                }

                tok = _update_token(&parse_info);
                if (tok->type != CSortTokenImport) {
                    CSort_panic_tok(entity->csort, tok, "Expected import statement got '%.*s'", SV_len(tok->tok_view), SV_data(tok->tok_view));
                }

                _parse_import_after_from(entity, &parse_info, _from_import);
                if (! is_already_kept) {
                    CSortEntity_append_module(entity, _from_import, parse_info.line_counter);
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

#define get_imports_start_line(X) ((X)->modules->line_in_file - 1)
#define _buffer_whitespace(X, Y) fprintf((X), "%*c", (Y), ' ')
#define _buffer_newline(X, Y) (fputs("\n", (X)), _buffer_whitespace(X, Y))

internal void
nowrap_imports(FILE* fp, CSortModuleObjNode* module) {
    CSortMemArenaNode** str_node;
    for (u32 i = 0; i < module->imports.len - 1; ++i) {
        str_node = (CSortMemArenaNode**) DynArray_get(&module->imports, i);
        fprintf(fp, "%s, ", (char*) (*str_node)->mem);
    }

    str_node = (CSortMemArenaNode**) DynArray_get(&module->imports, module->imports.len - 1);
    fprintf(fp, "%s", (char*) (*str_node)->mem);
    fputc('\n', fp);
    return;
}


internal void
wrap_imports(FILE* fp, const CSortConfig* conf, CSortModuleObjNode* m, const u32* import_offset) {
    u32 count = 0;
    CSortMemArenaNode** str_node;

    fputc('(', fp);
    for (; count < conf->wrap_after_n_imports; ++count) {
        str_node = (CSortMemArenaNode**) DynArray_get(&m->imports, count);
        fprintf(fp, "%s, ", (char*) (*str_node)->mem);
    }

    u32 remaining = m->imports.len - count;
    assert(conf->import_on_each_wrap != 0);
    while (remaining > conf->import_on_each_wrap) {
        _buffer_newline(fp, *import_offset);
        FOR (i, conf->import_on_each_wrap) {
            str_node = (CSortMemArenaNode**) DynArray_get(&m->imports, count + i);
            fprintf(fp, "%s, ", (char*) (*str_node)->mem);
        }

        remaining -= conf->import_on_each_wrap;
        count += conf->import_on_each_wrap;
    }

    // print remaining imports which weren't wrapped
    _buffer_newline(fp, *import_offset);
    for (; count < m->imports.len - 1; ++count) {
        str_node = (CSortMemArenaNode**) DynArray_get(&m->imports, count);
        fprintf(fp, "%s, ", (char*) (*str_node)->mem);
    }
    str_node = (CSortMemArenaNode**) DynArray_get(&m->imports, m->imports.len - 1);
    fprintf(fp, "%s)", (char*) (*str_node)->mem);
    fputc('\n', fp);
}


// prints processed/formatted imports
void
CSortEntity_do(CSortEntity* entity) {
    CSortEntity_sort(entity);
    CSort* csort = entity->csort;
    FILE* output_file = stdout;
    if (csort->conf.cmd_options.show_after_sort) {
        output_file = stdout;
    } else {
        return;
    }

    const CSortConfig* conf = &csort->conf;
    CSortMemArenaNode** str_node;

    for (CSortModuleObjNode* ptr = entity->modules, * module = ptr; ptr; module = ptr) {
        ptr = ptr->next;
        u32 import_offset = -3;                // Get offset little bit where the import keywords start!

        if (module->module_kind == CSortModuleKind_FROM) {
            import_offset += fprintf(output_file, "from %s import ", (char*) module->title->mem);
        } else if (module->module_kind == CSortModuleKind_IMPORT) {
            import_offset += fprintf(output_file, "import ");
        }

        _sort_imports(module);
        if (! conf->disable_wrapping) {
            if (conf->wrap_after_n_imports && module->imports.len > conf->wrap_after_n_imports) {
                wrap_imports(output_file, conf, module, &import_offset);
            } else nowrap_imports(output_file, module);
        } else {
            nowrap_imports(output_file, module);
        }
    }
}
