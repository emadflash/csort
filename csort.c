#include "csort.h"


inline CSort
CSort_mk(const String_View fileName, FILE* file) {
    return (CSort) {
        .fileName = fileName,
        .file = file,
        .arena = CSortMemArena_mk(),
    };
}


inline void
CSort_free(CSort* csort) {
    CSortMemArena_free(&(csort->arena));
    fclose(csort->file);
}


inline void
CSort_panic(CSort* csort, const char* msg) {
    eprintln("panic: %s", msg);
    CSort_free(csort);
}




// --------------------------------------------------------------------------------------------
void CSort_sortit(CSort* csort) {
    char buf[512];
    String_View buf_view = {0};

    while (fgets(buf, 512, csort->file)) {
        buf_view = SV(buf);
        buf_view.data[buf_view.len - 1] = '\0';
        buf_view.len -= 1;                                      // remove '\n'
    
    }
}
