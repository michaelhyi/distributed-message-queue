// TODO: add timeouts
// TODO: test outputs should be formatted
// TODO: test cases should have no logs other than results

#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>

#include "util.h"

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KCYN "\x1B[36m"

#define assert(expr)                                                           \
    if (!(expr)) {                                                             \
        fprintf(stderr, "Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, \
                strerror(errno));                                              \
        return -1;                                                             \
    }

struct test_case {
    char *name;
    void (*setup)(void);
    void (*teardown)(void);
    int (*test_case)(void);
};

struct test_suite {
    char *name;
    void (*setup)(void);
    void (*teardown)(void);
};

extern struct test_case tests[];
extern struct test_suite suite;

#define run_suite()                                                            \
    unsigned int passed = 0;                                                   \
    printf("Running Test Suite: " KCYN "%s\n" KNRM, suite.name);               \
    if (suite.setup)                                                           \
        suite.setup();                                                         \
    for (int i = 0; i < arrlen(tests); i++) {                                  \
        if (tests[i].setup)                                                    \
            tests[i].setup();                                                  \
        printf("Running %s...", tests[i].name);                                \
        if (tests[i].test_case() >= 0) {                                       \
            passed++;                                                          \
            printf(KGRN " passed!\n" KNRM);                                    \
        } else {                                                               \
            printf(KRED " failed!\n" KNRM);                                    \
        }                                                                      \
        if (tests[i].teardown)                                                 \
            tests[i].teardown();                                               \
    }                                                                          \
    if (suite.teardown)                                                        \
        suite.teardown();                                                      \
    printf("Score: " KCYN "%d/%d\n" KNRM, passed, arrlen(tests));              \
    if (passed != arrlen(tests))                                               \
        return 1;                                                              \
    return 0;

#endif
