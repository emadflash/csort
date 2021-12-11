#include "core.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>


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

void* DEV_malloc(bsize chunk_size, bsize chunk_len) {
    void* buff = malloc(chunk_size * chunk_len);
    if (! buff) die("malloc");
    return buff;
}

void* DEV_realloc(void* prev, bsize chunk_size, bsize chunk_len) {
    void* buff = realloc(prev, chunk_size* chunk_len);
    if (! buff) die("realloc");
    return buff;
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
// ~str utilities
char* str_find(char* begin, char* end, char c) {
    for (char* p = begin; p != end; ++p) {
        if (*p == c) return p;
    }

    return end;
}

char* str_findFirstNotOf(char* begin, char* end, char c) {
    for (char* p = begin; p != end; ++p) {
        if (*p != c) return p;
    }

    return begin;
}

char* str_find_backward(char* begin, char* end, char c) {
    for (char* p = end - 1; p != begin - 1; --p) {
        if (*p == c) return p;
    }

    return end;
}

char* str_findFirstNotOf_backward(char* begin, char* end, char c) {
    for (char* p = end - 1; p != begin - 1; --p) {
        if (*p != c) return p;
        --p;
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

String_View SV_buff(char* data, bsize len) {
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
    char* p = SV_findFirstNotOf_backward(sv, c);
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
string_alloc(String* s, bsize required_length) {
    s->len = s->memory_filled = 0;
    s->memory_size = s->memory_left = required_length + (1<<4);
    s->data = (char*) DEV_malloc(s->memory_size, sizeof(char));
}

internal void
string_realloc(String* s, bsize required_length) {
    assert(required_length > s->memory_left);
    s->memory_size += required_length + (1<<6);
    s->data = (char*) DEV_malloc(s->memory_size, sizeof(char));
}

internal int
string_fill(String* s, char* string, bsize string_length) {
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
string(char* buf, bsize len) {
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
