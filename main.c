#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "csort.h"

#include <fcntl.h>
#include <unistd.h>

#define csort_config_usr ".csortconfig"

internal inline const char*
_config_file_to_load(void) {
    return DEV_bool(access(csort_config_usr, F_OK) < 0) ? NULL : csort_config_usr;
}


internal CSortOptObj*
CSort_update_config_via_cmd(CSort* csort, u32* options_len) {
    CSortMemArenaNode* mem = CSortMemArena_alloc(&csort->arena);
    *options_len = 5;
    CSortOptObj options[] = {
        CSortOptBool(csort, &csort->conf.cmd_options.show_after_sort, "--show", "-s", "show changes after sanitizing"),
        CSortOptBool(csort, &csort->conf.cmd_options.recursive_apply, "--recur", "-r", "recursively iterates the whole directory, vaild if supplied path is a directory"),
        CSortOptBool(csort, &csort->conf.disable_wrapping, "--disable-wrapping", "-dw", "disable wrapping for duplicate librarys"),
        CSortOptBool(csort, &csort->conf.squash_for_duplicate_library, "--no-squash-duplicates", "-sd", "disable squashing duplicate librarys"),
        CSortOptInt(csort, &csort->conf.wrap_after_n_imports, "--wrap-after", "-wa", "starts wrapping imports after n, imports")
    };

    CSortMemArenaNode_fill(mem, options, sizeof(options));
    return (CSortOptObj*) mem->mem;
}

// --------------------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    CSort csort = CSort_mk();
    CSort_init_config(&csort, _config_file_to_load());

    u32 options_len = 0;
    CSortOptObj* options = CSort_update_config_via_cmd(&csort, &options_len);

    if (argc <= 1) {
        eprintln("usage: csort [FILE] [options..]");
        CSortOptParse_show_usage(stderr, options, options_len);
        exit(1);
    }

    const char* input_filepath = argv[1];
    if (CSortOptParse_is_help_flag(input_filepath)) {
        eprintln("usage: csort [FILE] [options..]");
        CSortOptParse_show_usage(stderr, options, options_len);
        exit(1);
    }
    CSortOptParse(argc - 1, &argv[2], options, options_len, "usage: csort [FILE] [options..]");

    if (! CSortDir_is_directory(&csort, input_filepath)) {
        CSortEntity entity = CSortEntity_mk(&csort, input_filepath);
        CSortEntity_do(&entity);
        CSortEntity_deinit(&entity);
    } else {
        CSortDir dir = CSortDir_mk(&csort, input_filepath);
        if (csort.conf.cmd_options.recursive_apply) {
            CSortDir_recur(&dir);
        }
    }

    CSort_deinit(&csort);
    return 0;
}
