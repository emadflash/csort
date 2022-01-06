#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "csort.h"

// Should we load config file present in current directory ?
#include <fcntl.h>
#include <unistd.h>

#define CURR_CONFIG ".csortconfig"
#define CONFIG_LUA_DEFAULT "config.lua"

internal inline char*
_config_file_to_load(void) {
    return  DEV_bool(access(CURR_CONFIG, F_OK) < 0) ? CONFIG_LUA_DEFAULT : CURR_CONFIG;
}



// --------------------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        eprintln("usage: csort {FILE}");
        exit(1);
    }

    const char* fileName = argv[1];
    FILE* fp = fopen(fileName, "r");
    if (! fp) {
        eprintln("error: failed to open '%s'", fileName);
        exit(1);
    }

    CSort csort = CSort_mk(SV(fileName), fp, _config_file_to_load());
    CSort_loadConfig(&csort);
    CSort_sortit(&csort);
    CSort_do(&csort);
    CSort_free(&csort);

    return 0;
}
