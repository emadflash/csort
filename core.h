#ifndef __CORE_H__
#define __CORE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define internal static
#define varPersist static
#define varGlobal static
#define bsize unsigned int


// --------------------------------------------------------------------------------------------
// ~Define function as internal in case of header file
#ifdef __LS_HEADER__
#   define function internal
#else
#   define function
#endif


// --------------------------------------------------------------------------------------------
// ~Types
typedef unsigned int uint;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef signed int iint;
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

// --------------------------------------------------------------------------------------------
// ~DEV macros
#define DEV_strIsEq(X, Y) (strcmp((X), (Y)) == 0)
#define DEV_strIsEqN(X, N1, Y, N2) (((N1) == (N2)) && (strncmp((X), (Y), N2) == 0))
#define DEV_memzero(X, Y) memset((X), 0, (Y))
#define DEV_bool(EXPR) ((EXPR) ? true : false)
#define DEV_panic(X) (eprintln("panic:"#X), exit(1))

#define FOR(X, Y) for (int (X) = 0; (X) < (Y); ++(X))

// --------------------------------------------------------------------------------------------
// ~utilites
extern void die(char *fmt, ...);
extern FILE* DEV_fopen(const char* file, const char* mode);
extern void* DEV_malloc(bsize chunk_size, bsize chunk_len);
extern void* DEV_realloc(void* prev, bsize chunk_size, bsize chunk_len); 


// --------------------------------------------------------------------------------------------
// ~print utilites
#define println(...) DEV_println(stdout, __VA_ARGS__)
#define eprintln(...) DEV_println(stderr, __VA_ARGS__)
#define vfprintln(X, Y, Z)\
    vfprintf(X, Y, Z);    \
    fputs("\n", X)

extern void DEV_println(FILE* fp, char* fmt, ...);



// --------------------------------------------------------------------------------------------
// ~Memory arena node 
typedef struct CSortMemArenaNode CSortMemArenaNode;
struct CSortMemArenaNode {
    void* mem;
    u8*   mem_cursor;
    u32   mem_used,
          mem_free,
          mem_size;

    struct CSortMemArenaNode* next;
    struct CSortMemArenaNode* prev;
};

extern void CSortMemArenaNode_init(CSortMemArenaNode* node);
extern void CSortMemArenaNode_free(CSortMemArenaNode* node);
extern void CSortMemArenaNode_fill(CSortMemArenaNode* node, void* data, u32 data_size);



// --------------------------------------------------------------------------------------------
// ~allocator
typedef struct CSortMemArena CSortMemArena;
struct CSortMemArena {
    CSortMemArenaNode* first;
    CSortMemArenaNode* head;
};

extern CSortMemArena CSortMemArena_mk();
extern void CSortMemArena_free(CSortMemArena* arena);
extern CSortMemArenaNode* CSortMemArena_alloc(CSortMemArena* arena);
extern void CSortMemArena_dealloc(CSortMemArena* arena, CSortMemArenaNode* node);



// --------------------------------------------------------------------------------------------
// ~str utilities
extern char* str_find(char* begin, char* end, char c);
extern char* str_findFirstNotOf(char* begin, char* end, char c);
extern char* str_findFirstNotOf_backward(char* begin, char* end, char c);


// --------------------------------------------------------------------------------------------
// ~String_View
#define SV_print(X) fprintf(stdout, "[data: %.*s, size: %ld]", (X).len, (X).data, (X).len)
#define SV_arg(X) ((X).data)
#define SV_len(X) ((X).len)
#define SV_begin(X) ((X).data)
#define SV_end(X) ((X).data + (X).len)
#define SV_ref(X) (&(X))

#define SV_isEq(X, Y) DEV_strIsEqN((X).data, (X).len, (Y).data, (Y).len)
#define SV_isEqRaw(X, Y) DEV_strIsEqN((X).data, (X).len, (Y), strlen((Y)))
#define SV_find(X, Y) str_find(SV_begin(X), SV_end(X), Y)
#define SV_find_backward(X, Y) str_find_backward(SV_begin(X), SV_end(X), Y)
#define SV_findFirstNotOf(X, Y) str_findFirstNotOf(SV_begin(X), SV_end(X), Y)
#define SV_findFirstNotOf_backward(X, Y) str_findFirstNotOf_backward(SV_begin(X), SV_end(X), Y)

typedef struct String_View String_View;
struct String_View {
    char* data;
    bsize len;
};

extern String_View SV(const char* data);
extern String_View SV_buff(char* data, bsize len);
extern inline String_View SV_slice(char* first, char* last);
extern String_View SV_chop(String_View sv, char c);
extern String_View SV_chop_backward(String_View sv, char c);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~String
typedef struct String String;
struct String {
    char* data;
    bsize len;
    bsize memory_size;
    bsize memory_filled;
    bsize memory_left;
};


// ~macro functions
#define string_raw(X) string((X), strlen((X)))
#define string_fromSV(X) string((X).data, (X).len)

internal void string_alloc(String* s, bsize required_length);
internal void string_realloc(String* s, bsize required_length);
internal int string_fill(String* s, char* string, bsize string_length);

extern String string(char* buf, bsize len);
extern inline void string_free(String* s);
extern String string_slice(const char* begin, const char* end);
extern String_View SV_fromString(const String* s);
extern String_View string_toSV(const String* s);

#endif
