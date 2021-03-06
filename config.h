#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "core.h"
#include "external/lua/lua.h"
#include "external/lua/lualib.h"
#include "external/lua/lauxlib.h"

#include <stdbool.h>

/*
 * List of @param(know_standard_library) 
 * Copied from `isort's` list using `isort --show-config`
 */
#define know_standard_library_len 218
internal char know_standard_library[know_standard_library_len][256] = {
    "http",/*# {{{*/
    "os",
    "sre_compile",
    "tabnanny",
    "binhex",
    "shelve",
    "operator",
    "imp",
    "hmac",
    "inspect",
    "cmath",
    "subprocess",
    "asynchat",
    "timeit",
    "binascii",
    "sre_constants",
    "bisect",
    "reprlib",
    "tracemalloc",
    "xdrlib",
    "optparse",
    "ossaudiodev",
    "ensurepip",
    "dbm",
    "mmap",
    "cgi",
    "itertools",
    "pprint",
    "plistlib",
    "pickletools",
    "contextlib",
    "getopt",
    "math",
    "pickle",
    "array",
    "tkinter",
    "shlex",
    "warnings",
    "crypt",
    "telnetlib",
    "textwrap",
    "tempfile",
    "functools",
    "collections",
    "marshal",
    "pwd",
    "graphlib",
    "codecs",
    "sched",
    "grp",
    "imghdr",
    "_dummy_thread",
    "aifc",
    "json",
    "multiprocessing",
    "test",
    "csv",
    "netrc",
    "decimal",
    "io",
    "symtable",
    "stringprep",
    "unittest",
    "readline",
    "time",
    "zoneinfo",
    "zlib",
    "cmd",
    "pipes",
    "quopri",
    "fpectl",
    "secrets",
    "bz2",
    "email",
    "bdb",
    "turtle",
    "builtins",
    "tokenize",
    "msvcrt",
    "contextvars",
    "select",
    "gettext",
    "threading",
    "keyword",
    "pathlib",
    "lib2to3",
    "nis",
    "py_compile",
    "ast",
    "ipaddress",
    "audioop",
    "encodings",
    "cgitb",
    "trace",
    "abc",
    "re",
    "distutils",
    "signal",
    "html",
    "shutil",
    "webbrowser",
    "_thread",
    "faulthandler",
    "fcntl",
    "doctest",
    "xml",
    "pydoc",
    "ftplib",
    "stat",
    "colorsys",
    "atexit",
    "copy",
    "site",
    "gc",
    "concurrent",
    "copyreg",
    "numbers",
    "posix",
    "uuid",
    "modulefinder",
    "wsgiref",
    "tty",
    "configparser",
    "poplib",
    "socketserver",
    "smtpd",
    "urllib",
    "filecmp",
    "glob",
    "syslog",
    "msilib",
    "nntplib",
    "locale",
    "selectors",
    "ctypes",
    "ntpath",
    "dataclasses",
    "queue",
    "hashlib",
    "profile",
    "compileall",
    "mimetypes",
    "wave",
    "statistics",
    "zipapp",
    "socket",
    "codeop",
    "rlcompleter",
    "dis",
    "struct",
    "symbol",
    "errno",
    "mailcap",
    "types",
    "sunau",
    "importlib",
    "venv",
    "typing",
    "code",
    "linecache",
    "pty",
    "resource",
    "lzma",
    "chunk",
    "gzip",
    "turtledemo",
    "termios",
    "calendar",
    "sre_parse",
    "sysconfig",
    "fractions",
    "sre",
    "string",
    "random",
    "macpath",
    "traceback",
    "enum",
    "mailbox",
    "pkgutil",
    "sqlite3",
    "zipfile",
    "ssl",
    "token",
    "smtplib",
    "getpass",
    "winreg",
    "xmlrpc",
    "pdb",
    "runpy",
    "spwd",
    "parser",
    "logging",
    "imaplib",
    "sys",
    "sndhdr",
    "platform",
    "unicodedata",
    "dummy_threading",
    "asyncore",
    "base64",
    "pstats",
    "heapq",
    "formatter",
    "difflib",
    "posixpath",
    "curses",
    "pyclbr",
    "weakref",
    "winsound",
    "uu",
    "zipimport",
    "cProfile",
    "datetime",
    "tarfile",
    "argparse",
    "fnmatch",
    "asyncio",
    "fileinput"/*# }}}*/
};

/*
 * List of @param(skip_directories), which we'll skip while traversing filesystem
 */
#define skip_directories_len 2
internal char skip_directories[skip_directories_len][28] = {
    "build",
    ".git",
};

/*
 * List of @param(file_exts), on files with these extensions we will run #csort
 */
#define file_exts_len 1
internal char file_exts[file_exts_len][22] = {
    ".py"
};


// --------------------------------------------------------------------------------------------
typedef struct CSortConfigCmd CSortConfigCmd;
struct CSortConfigCmd {
    bool show_after_sort, recursive_apply;
    char* input_filepath;
};

// --------------------------------------------------------------------------------------------
typedef struct CSortConfig CSortConfig;
struct CSortConfig {
    CSortMemArena* arena;
    lua_State* lua;
    CSortConfigCmd cmd_options;             // Options which are read from cmdline

    DynArray know_standard_library, skip_directories, file_exts;
    bool squash_for_duplicate_library,
         disable_wrapping;
    u64 wrap_after_n_imports,
        import_on_each_wrap,
        wrap_after_col;
};


int CSortConfig_init(CSortConfig* config, CSortMemArena* arena);
int CSortConfig_init_w_lua(CSortConfig* config, CSortMemArena* arena, const char* config_file_lua);
void CSortConfig_deinit(CSortConfig* config);
bool CSortConfigFindStrList(CSortConfig* conf, int which_list, const char* match);
int array_push_from_str(DynArray* array, lua_State* lua, const char* table_name);

#endif
