#ifndef __CSORT_H__
#define __CSORT_H__

#include <stdio.h>
#include "core.h"


typedef struct CSort CSort;
struct CSort {
    FILE* file;
    String_View fileName;

    CSortMemArena arena;
};

extern inline CSort CSort_mk(const String_View fileName, FILE* file);
extern inline void CSort_free(CSort* csort);
extern inline void CSort_panic(CSort* csort, const char* msg);

extern void CSort_sortit(CSort* csort);


#endif
