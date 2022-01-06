#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "csort.h"

// Should we load config file present in current directory ?
#include <fcntl.h>
#include <unistd.h>

#define csort_config_usr ".csortconfig"

internal inline char*
_config_file_to_load(void) {
    return DEV_bool(access(csort_config_usr, F_OK) < 0) ? NULL : csort_config_usr;
}



// --------------------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        eprintln("usage: csort {FILE}");
        exit(1);
    }

    const char* fileName = argv[1];
    CSort csort = CSort_mk(fileName, _config_file_to_load());
    CSort_sortit(&csort);
    CSort_do(&csort);
    CSort_free(&csort);

    return 0;
}
