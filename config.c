#include "config.h"
#include <assert.h>



// --------------------------------------------------------------------------------------------
int
CSortConfig_init_w_lua(CSortConfig* config, CSortMemArena* arena, const char* config_file_lua) {
    config->cmd_options = (CSortConfigCmd) {0};
    config->arena = arena;
    config->lua = luaL_newstate();
    config->know_standard_library = DynArray_mk(sizeof(String));
    config->skip_directories = DynArray_mk(sizeof(String));
    int luaResult = luaL_dofile(config->lua, config_file_lua);
    if (luaResult != LUA_OK) {
        lua_close(config->lua);
        return -1;
    }

    return 0;
}

int
CSortConfig_init(CSortConfig* config, CSortMemArena* arena) {
    // know_standard_library
    config->know_standard_library = DynArray_mk(sizeof(String));
    FOR (i, know_standard_library_len) {
        const char* str = know_standard_library[i];
        const String lib = string((char*) str, strlen(str));
        DynArray_push(&config->know_standard_library, (void*) &lib);
    }
    qsort(config->know_standard_library.mem, config->know_standard_library.len, sizeof(String), string_strncmp);

    // skip_directories 
    config->skip_directories = DynArray_mk(sizeof(String));
    FOR (i, skip_directories_len) {
        const char* str = skip_directories[i];
        const String dir_name = string((char*) str, strlen(str));
        DynArray_push(&config->skip_directories, (void*) &dir_name);
    }

    config->cmd_options = (CSortConfigCmd) {0};
    config->squash_for_duplicate_library = true;
    config->disable_wrapping = false;
    config->wrap_after_n_imports = 4;
    config->import_on_each_wrap = 9;
    config->wrap_after_col = 50;
    config->wrap_after_col = 80;
    return 0;
}

inline bool
CSortConfig_findInKnowLibrarys(const CSortConfig* conf, const String_View library_view) {
    return (! bsearch(&library_view.data, conf->know_standard_library.mem, conf->know_standard_library.len, sizeof(String), string_strncmp))
        ? false : true;
}

void
CSortConfig_free(CSortConfig* config) {
    if (config->lua) lua_close(config->lua);
    FOR (i, config->know_standard_library.len) {
        String* s = (String*) DynArray_get(&config->know_standard_library, i);
        string_free(s);
    }
    FOR (i, config->skip_directories.len) {
        String* s = (String*) DynArray_get(&config->skip_directories, i);
        string_free(s);
    }
    DynArray_free(&config->know_standard_library);
    DynArray_free(&config->skip_directories);
}

int
array_push_from_str(DynArray* array, lua_State* lua, const char* table_name) {
    lua_getglobal(lua, table_name);
    if (! lua_istable(lua, -1)) {
        return -1;
    }

    lua_pushnil(lua);
    while (lua_next(lua, -2) != 0) {
        assert(lua_type(lua, -1) == LUA_TSTRING);
        const char* cstr = lua_tostring(lua, -1);
        const String str = string((char*) cstr, strlen(cstr));
        DynArray_push(array, (void*) &str);
        lua_pop(lua, 1);
    }
    return 0;
}
