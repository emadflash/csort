#include <stdio.h>
#include "../core.h"
#include "../csort.h"
#include "check.h"

typedef struct sample_struct sample_struct;
struct sample_struct { int data; };

int main() {
    CHECK_Init();
    
    /* -------------------------------------------------------------------------------------------- */
    TEST(CSort_nexttoken_for_identifiers) {
        const char* fileName = "./test/example_test_python.py";
        FILE* fp = fopen(fileName, "r");
        if (! fp) {
            eprintln("error: failed to open '%s'", fileName);
            exit(1);
        }
        
        // TODO(emf): implement test 

        fclose(fp);
    }

    /* -------------------------------------------------------------------------------------------- */
    TEST(DynArray_functions) {
        DynArray a = DynArray_mk(sizeof(sample_struct));
        sample_struct s1 = { .data = 59 };
        sample_struct s2 = { .data = 69 };
        DynArray_push(&a, (void*) &s1);
        DynArray_push(&a, (void*) &s2);

        CHECK_INT(2, a.len);
        CHECK_INT(59, ((sample_struct*) DynArray_get(&a, 0))->data);
        CHECK_INT(69, ((sample_struct*) DynArray_get(&a, 1))->data);
        CHECK_INT((1 << 4) - 2, a.mem_free);
        DynArray_free(&a);
    }


    /* -------------------------------------------------------------------------------------------- */
    TEST(DEV_strToInt) {
        u64 n1 = 0;
        const char* n1_string = "69";
        DEV_strToInt(n1_string, &n1, 10);
        CHECK_INT(n1, 69);

        u64 n2 = 0;
        const char* n2_string = "1111169";
        DEV_strToInt(n2_string, &n2, 10);
        CHECK_INT(n2, 1111169);
    }

    /* -------------------------------------------------------------------------------------------- */
    TEST(_compare_cstr_nodes) {
        CSortMemArena arena = CSortMemArena_mk();
        CSortMemArenaNode* n1 = CSortMemArenaCopyCStr(&arena, "lib10", 5);
        CSortMemArenaNode* n2 = CSortMemArenaCopyCStr(&arena, "lib110", 6);

        CHECK_INT(0, _compare_cstr_nodes(&n1, &n2));

        CSortMemArena_free(&arena);
    }

    CHECK_Deinit();
    return 0;
}
