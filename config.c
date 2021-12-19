#include "config.h"



// --------------------------------------------------------------------------------------------
int
CSortConfig_init(CSortConfig* config, CSortMemArena* arena, const char* config_file_lua) {
    config->arena = arena;
    config->lua = luaL_newstate();
    int luaResult = luaL_dofile(config->lua, config_file_lua);
    if (luaResult != LUA_OK) {
        lua_close(config->lua);
        return -1;
    }

    return 0;
}

inline int
CSortConfig_free(CSortConfig* config) {
    lua_close(config->lua);
}
