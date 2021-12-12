#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "csort.h"


// --------------------------------------------------------------------------------------------
int main() {
    const char* fileName = "example-python.py";
    FILE* fp = fopen(fileName, "r");
    if (! fp) {
        eprintln("error: failed to open '%s'", fileName);
        exit(1);
    }

    CSort csort = CSort_mk(SV(fileName), fp);

    CSort_sortit(&csort);
    CSort_print_imports(&csort);
    CSort_free(&csort);

    return 0;
}
