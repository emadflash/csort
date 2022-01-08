#include "core.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>


// --------------------------------------------------------------------------------------------
// ~utilites
void die(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs(": ", stderr);
    perror(NULL);
    exit(1);
}

FILE* DEV_fopen(const char* file, const char* mode) {
    FILE* fp = fopen(file, mode);
    if (!fp) die("fopen");
    return fp;
}

void* DEV_malloc(u32 chunk_size, u32 chunk_len) {
    void* buff = malloc(chunk_size * chunk_len);
    if (! buff) die("malloc");
    return buff;
}

void* DEV_realloc(void* prev, u32 chunk_size, u32 chunk_len) {
    void* buff = realloc(prev, chunk_size* chunk_len);
    if (! buff) die("realloc");
    return buff;
}


int
DEV_strToInt(const char* str, u64* strInt, int base) {
   char* endptr;
   errno = 0;
   *strInt = strtol(str, &endptr, base);

   if ((errno == ERANGE && (*strInt == LLONG_MAX || *strInt == LONG_MIN))
           || (errno != 0 && *strInt == 0)) {
       return -1;
   }

   if (endptr == str) {
       return -1;
   }
   return 0;
}

// --------------------------------------------------------------------------------------------
// ~print utilites
void DEV_println(FILE* fp, char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintln(fp, fmt, ap);
    va_end(ap);
}



// --------------------------------------------------------------------------------------------
// ~memory arena node 
void
CSortMemArenaNode_init(CSortMemArenaNode* node) {
    node->mem_size = 512;
    node->mem = (void*) DEV_malloc(node->mem_size, 1);
    node->mem_cursor = (u8*) node->mem;
    node->mem_free = 512;
    node->mem_used = 0;
}

void
CSortMemArenaNode_init_w_size(CSortMemArenaNode* node, u32 size) {
    node->mem_size = 512;
    node->mem = (void*) DEV_malloc(node->mem_size, 1);
    node->mem_cursor = (u8*) node->mem;
    node->mem_free = 512;
    node->mem_used = 0;
}

void
CSortMemArenaNode_free(CSortMemArenaNode* node) {
    free(node->mem);
}


void
CSortMemArenaNode_fill(CSortMemArenaNode* node, void* data, u32 data_size) {
    if (data_size >= node->mem_free) {
        node->mem_size += node->mem_free += (data_size + 512);
        node->mem = (void*) DEV_realloc(node->mem, 1, node->mem_size);
        node->mem_cursor = node->mem + node->mem_used;
    }

    memcpy(node->mem_cursor, data, data_size);
    node->mem_cursor += data_size;
    node->mem_used += data_size;
    node->mem_free -= data_size;
}



// --------------------------------------------------------------------------------------------
// ~allocator
CSortMemArena CSortMemArena_mk() {
    return (CSortMemArena) {
        .first = NULL,
        .head = NULL,     
    };
}

void
CSortMemArena_free(CSortMemArena* arena) {
    for (CSortMemArenaNode* ptr = arena->first, * tmp = ptr; ptr; tmp = ptr) {
        ptr = ptr->next;
        CSortMemArenaNode_free(tmp);
        free(tmp);
    }
}

CSortMemArenaNode*
CSortMemArena_alloc(CSortMemArena* arena) {
    CSortMemArenaNode* node = (CSortMemArenaNode*) DEV_malloc(1, sizeof(CSortMemArenaNode));
    if (arena->head == NULL) {
        arena->head = node;
        arena->first = node;
        node->prev = NULL;
    } else {
        node->prev = arena->head;
        arena->head->next = node;
        arena->head = node;
    }

    CSortMemArenaNode_init(node);
    node->next = NULL;
    return node;
}

void
CSortMemArena_dealloc(CSortMemArena* arena, CSortMemArenaNode* node) {
    CSortMemArenaNode* prev_node = node->prev;
    CSortMemArenaNode* next_node = node->next;
    if (! prev_node) {
        arena->first = node->next;
    } else {
        prev_node->next = next_node;
    }

    if (! next_node) {
        arena->head = prev_node;
    } else {
        next_node->prev = prev_node;
    }
    CSortMemArenaNode_free(node);
    free(node);
}


inline CSortMemArenaNode*
CSortMemArenaCopyCStr(CSortMemArena* arena, char* string, u32 string_size) {
    CSortMemArenaNode* n = CSortMemArena_alloc(arena);
    CSortMemArenaNode_fill(n, (void*) string, string_size);
    CSortMemArenaNode_fill(n, (void*)"\0", 1);

    return n;
}


// --------------------------------------------------------------------------------------------
// ~str utilities
char* str_find(char* begin, char* end, char c) {
    for (char* p = begin; p != end; ++p) {
        if (*p == c) return p;
    }

    return end;
}

char* str_findPred(char* begin, char* end, int (*predicate)(int)) {
    for (char* p = begin; p != end; ++p) {
        if (predicate(*p)) return p;
    }

    return begin;
}

char* str_findFirstNotOf(char* begin, char* end, char c) {
    for (char* p = begin; p != end; ++p) {
        if (*p != c) return p;
    }
    return begin;
}

char* str_findFirstNotOfPred(char* begin, char* end, int (*predicate)(int)) {
    for (char* p = begin; p != end; ++p) {
        if (! predicate(*p)) return p;
    }
    return begin;
}

char* str_findRev(char* begin, char* end, char c) {
    for (char* p = end - 1; p != begin - 1; --p) {
        if (*p == c) return p;
    }
    return end;
}

char* str_findPredRev(char* begin, char* end, int (*predicate)(int)) {
    for (char* p = end - 1; p != begin - 1; --p) {
        if (predicate(*p)) return p;
    }
    return end;
}

char* str_findFirstNotOfRev(char* begin, char* end, char c) {
    for (char* p = end - 1; p != begin - 1; --p) {
        if (*p != c) return p;
    }
    return begin;
}

char* str_findFirstNotOfPredRev(char* begin, char* end, int (*predicate)(int)) {
    for (char* p = end - 1; p != begin - 1; --p) {
        if (! predicate(*p)) return p;
    }
    return begin;
}

// --------------------------------------------------------------------------------------------
// ~String_View
String_View SV(const char* data) {
    return (String_View) {
        .data = data,
        .len = strlen(data),
    };
}

String_View SV_buff(char* data, u32 len) {
    return (String_View) {
        .data = data,
        .len = len,
    };
}

inline String_View SV_slice(char* first, char* last) {
    return (String_View) {
        .data = first,
        .len = last - first,
    };
}

String_View SV_chop(String_View sv, char c) {
    char* p = SV_findFirstNotOf(sv, c);
    if (p == SV_begin(sv)) {
        return sv;
    }

    return (String_View) {
        .data = p,
        .len = SV_end(sv) - p,
    };
}

String_View SV_chop_backward(String_View sv, char c) {
    char* p = SV_findFirstNotOfRev(sv, c);
    if (p == SV_end(sv)) {
        return sv;
    }

    return (String_View) {
        .data = SV_begin(sv),
        .len = p - SV_begin(sv) + 1,
    };
}


// --------------------------------------------------------------------------------------------
// ~String
internal void
string_alloc(String* s, u32 required_length) {
    s->len = s->memory_filled = 0;
    s->memory_size = s->memory_left = required_length + (1<<4);
    s->data = (char*) DEV_malloc(s->memory_size, sizeof(char));
}

internal void
string_realloc(String* s, u32 required_length) {
    assert(required_length > s->memory_left);
    s->memory_size += required_length + (1<<6);
    s->data = (char*) DEV_malloc(s->memory_size, sizeof(char));
}

internal int
string_fill(String* s, char* string, u32 string_length) {
    if (string_length >= s->memory_left) {
        return -1;
    }
    memcpy(s->data + s->len, string, string_length);
    s->len += string_length;
    s->memory_filled = s->len + 1;
    s->memory_left = s->memory_size - s->memory_filled;
    s->data[s->len] = '\0'; 

    return 0;
}

inline String
string(char* buf, u32 len) {
    String s;
    string_alloc(&s, len);
    string_fill(&s, buf, len);
    return s;
}

inline void
string_free(String* s) {
    free(s->data);
}

String
string_slice(const char* begin, const char* end) {
    String s;
    string_alloc(&s, end - begin);
    string_fill(&s, begin, end - begin);

    return s;
}

int
string_strncmp(const String* s1, const String* s2) {
    return strncmp(s1->data, s2->data, s2->len);
}

String_View
SV_fromString(const String* s) {
    return (String_View) {
        .data = s->data,
        .len = s->len,
    };
}

String_View string_toSV(const String* s) {
    return (String_View) {
        .data = s->data,
        .len = s->len,
    };
}



// --------------------------------------------------------------------------------------------
DynArray
DynArray_mk(u32 chunk_size) {
    DynArray arr = {0};
    arr.mem_size = 1 << 4;
    arr.chunk_size = chunk_size;
    arr.mem = (void*) DEV_malloc(arr.mem_size, arr.chunk_size);
    arr.mem_cursor = (u8*) arr.mem;
    arr.mem_free = 1 << 4;
    arr.len = 0;
    return arr;
}

void
DynArray_free(DynArray* arr) {
    free(arr->mem);
}

inline void*
DynArray_get(DynArray* arr, u32 idx) {
    return (void*) ((u8*) arr->mem + arr->chunk_size * idx);
}

void
DynArray_push(DynArray* arr, void* data) {
    if (arr->len >= arr->mem_size) {
        arr->mem_size += 1 << 4;
        arr->mem = (void*) DEV_realloc(arr->mem, arr->chunk_size, arr->mem_size);
    }

    memcpy(arr->mem + arr->chunk_size* arr->len, data, arr->chunk_size);
    arr->len += 1;
    arr->mem_free -= 1;
    arr->mem_cursor += arr->chunk_size;
}

int
DynArray_pop(DynArray* arr) {
    if (arr->len <= 0) {
        return -1;
    }
    arr->len -= 1;
    return 0;
}
