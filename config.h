#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "core.h"
#include "external/lua/include/lua.h"
#include "external/lua/include/lualib.h"
#include "external/lua/include/lauxlib.h"

#include <stdbool.h>


typedef struct CSortConfig CSortConfig;
struct CSortConfig {
    CSortMemArena* arena;
    lua_State* lua;
};


extern int CSortConfig_init(CSortConfig* config, CSortMemArena* arena, const char* config_file_lua);
extern int CSortConfig_free(CSortConfig* config);

#endif
