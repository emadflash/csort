#ifndef __CHECK_H__
#define __CHECK_H__

#include <string.h>

typedef struct {
    unsigned int cases,
                 checks,
                 passes,
                 fails;

    char* currentCase;
} Check_Instance;

static Check_Instance Check_Instance_mk(unsigned int cases, unsigned int checks, unsigned int passes, unsigned int fails) {
    return (Check_Instance) {
        .cases = cases,
        .checks = checks,
        .passes = passes,
        .fails = fails,

        .currentCase = NULL,
    };
}


// --------------------------------------------------------------------------------------------
// Pretty colors
#define CHECK_label_failure "\033[1;31m[failed]\033[0m"
#define CHECK_label_failure_uponFailure(X) println("%s %s: "#X, (CHECK_label_failure), (checkInstance.currentCase))
#define CHECK_label_allTestsPassed "\033[1;31m\n\tAll test passed\033[0m\n"
#define CHECK_label_youHaveFailedTests "\033[1;31m\n\n\tYou have %d failed tests.\033[0m\n\n"



// --------------------------------------------------------------------------------------------
// Initialize Check_Instance
#define CHECK_Init() Check_Instance checkInstance = Check_Instance_mk(0, 0, 0, 0)
#define CHECK_Deinit()                                                                                    \
    if ((checkInstance.fails) == 0) println("\n%s", CHECK_label_allTestsPassed);                          \
    else {                                                                                                \
        fprintf(stdout, "Cases: %d, Checks: %d, Pass: %d, Fails: %d",                                     \
            (checkInstance.cases), (checkInstance.checks), (checkInstance.passes), (checkInstance.fails));\
        fprintf(stdout, CHECK_label_youHaveFailedTests, (checkInstance.fails));                           \
    }



// ~Macros for explicit fail or pass
#define FAIL() (println("%s %s", (CHECK_label_failure), (checkInstance.currentCase)), ++(checkInstance.checks), ++(checkInstance.fails))
#define PASS() (++(checkInstance.checks), ++(checkInstance.passes))
#define TEST(Y) (println("[testing] %s", #Y), (checkInstance.currentCase) = (#Y), ++(checkInstance.cases));


// ~checks a boolean expression
#define CHECK_EXPR(X)                                                                \
    if (! (X)) {                                                                     \
        println("%s %s: %s", (CHECK_label_failure), (checkInstance.currentCase), #X);\
        ++(checkInstance.checks);                                                    \
        ++(checkInstance.fails);                                                     \
    }                                                                                \
                                                                                     \
    PASS()


// ~checks a boolean expression, upon failure calls callback Y
#define CHECK_EXPR_callback(X, Y, ...)                               \
    if (! (X)) {                                                     \
        println("%s %s: %s", (CHECK_label_failure), (Check_Instance.currentCase), #X);\
        Y(__VA_ARGS__);                                              \
                                                                     \
        ++(checkInstance.checks);                                    \
        ++(checkInstance.fails);                                     \
    }                                                                \
                                                                     \
    PASS()

#define CHECK_STR(X, Y)                                                      \
    ++(checkInstance.checks);                                                \
                                                                             \
    if (strncmp((X), (Y), strlen((Y))) != 0) {                               \
        CHECK_label_failure_uponFailure(X == Y);                             \
        println("\t %s: %s", #X, (X));                                       \
        println("\t %s: %s", #Y, (Y));                                       \
                                                                             \
        ++(checkInstance.fails);                                             \
    } else ++(checkInstance.passes);                                         \

#define CHECK_SV(X, Y)                                                       \
    ++(checkInstance.checks);                                                \
                                                                             \
    if ((X).len != (Y).len) {                                                \
        CHECK_label_failure_uponFailure(X == Y);                             \
        println("\t %s: %s", #X, (X).data);                                  \
        println("\t %s: %s", #Y, (Y).data);                                  \
                                                                             \
        ++(checkInstance.fails);                                             \
    }                                                                        \
                                                                             \
    if (strncmp((X).data, (Y).data, (Y).len) != 0) {                         \
        CHECK_label_failure_uponFailure(X == Y);                             \
        println("\t %s: %s", #X, (X).data);                                  \
        println("\t %s: %s", #Y, (Y).data);                                  \
                                                                             \
        ++(checkInstance.fails);                                             \
    } else ++(checkInstance.passes);

#define CHECK_INT(X, Y)                                                      \
    ++(checkInstance.checks);                                                \
                                                                             \
    if ((X) != (Y)) {                                                        \
        CHECK_label_failure_uponFailure(X == Y);                             \
        println("\t %s: 0x%lx", #X, (X));                                    \
        println("\t %s: 0x%lx", #Y, (Y));                                    \
                                                                             \
        ++(checkInstance.fails);                                             \
    } else ++(checkInstance.passes);                                         \

#endif
