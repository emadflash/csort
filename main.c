#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "csort.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define csort_config_usr ".csortconfig"

internal inline const char*
_config_file_to_load(void) {
    return DEV_bool(access(csort_config_usr, F_OK) < 0) ? NULL : csort_config_usr;
}

// Make cmdline options attach to #CSortConfigCmd struct
// So, when we parse, we update #CSortConfigCmd struct
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

// check if #filepath is a directory.
internal int
is_directory(CSort* csort, const char* filepath, bool* success) {
    struct stat file_stat;
    if (stat(filepath, &file_stat) < 0) {
        log_error("is_directory: stat failed: %s: %s", filepath, strerror(errno));
        return -1;
    } 
    *success = ((file_stat.st_mode & S_IFMT) == S_IFDIR) ? true : false;
    return 0;
}

// Make entity for #input_filepath
// This function is a callback.
internal void
CSortHandlePyFile(CSort* csort, const char* input_filepath) {
    const String_View input_file_sv = SV(input_filepath);
    String_View input_file_ext = {0};
    if (CSortGetExtension(input_file_sv, &input_file_ext) == 0 &&
        CSortConfigFindStrList(&csort->conf, 2, input_file_ext.data)) {
        println("\033[1;31m%s:\033[0m", input_filepath);
        CSortEntity entity = CSortEntity_mk(csort, input_filepath);
        CSortEntity_do(&entity);
        CSortEntity_deinit(&entity);
        println("");
    }
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

    bool success = false;
    if (is_directory(&csort, input_filepath, &success) < 0) {
        log_error("csort: Couldn't check if %s is a directory.", input_filepath);
        CSort_deinit(&csort);
        exit(1);
    }

    if (! success) {
        CSortEntity entity = CSortEntity_mk(&csort, input_filepath);
        CSortEntity_do(&entity);
        CSortEntity_deinit(&entity);
    } else {
        if (csort.conf.cmd_options.recursive_apply) {
            CSortPerformOnFileCallbackRecur(&csort, input_filepath, CSortHandlePyFile);
        } else {
            CSortPerformOnFileCallback(&csort, input_filepath, CSortHandlePyFile);
        }
    }

    CSort_deinit(&csort);
    return 0;
}
