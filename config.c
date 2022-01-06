#include "config.h"



// --------------------------------------------------------------------------------------------
int
CSortConfig_init_w_lua(CSortConfig* config, CSortMemArena* arena, const char* config_file_lua) {
    config->arena = arena;
    config->lua = luaL_newstate();
    int luaResult = luaL_dofile(config->lua, config_file_lua);
    if (luaResult != LUA_OK) {
        lua_close(config->lua);
        return -1;
    }

    return 0;
}

int
CSortConfig_init(CSortConfig* config, CSortMemArena* arena) {
    config->know_standard_library = know_standard_library;
    config->squash_for_duplicate_library = true;
    config->disable_wrapping = false;
    config->wrap_after_n_imports = 4;
    config->import_on_each_wrap = 9;
    config->wrap_after_col = 50;
    config->wrap_after_col = 80;
}

inline int
CSortConfig_free(CSortConfig* config) {
    if (config->lua) lua_close(config->lua);
}
