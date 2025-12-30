#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>

#define assert(expr)                                                           \
    if (!(expr)) {                                                             \
        fprintf(stderr, "Assertion failed at %s:%d with error: %s\n",          \
                __FILE__, __LINE__, strerror(errno));                          \
        return -1;                                                             \
    }

struct test_case {
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
    printf("Running Test Suite: %s\n", suite.name);                            \
    if (suite.setup)                                                           \
        suite.setup();                                                         \
    for (int i = 0; i < arrlen(tests); i++) {                                  \
        if (tests[i].setup)                                                    \
            tests[i].setup();                                                  \
        if (tests[i].test_case() >= 0)                                         \
            passed++;                                                          \
        if (tests[i].teardown)                                                 \
            tests[i].teardown();                                               \
    }                                                                          \
    if (suite.teardown)                                                        \
        suite.teardown();                                                      \
    printf("Passed %d/%d tests\n", passed, arrlen(tests));                     \
    if (passed != arrlen(tests))                                               \
        return 1;                                                              \
    return 0;

#endif
