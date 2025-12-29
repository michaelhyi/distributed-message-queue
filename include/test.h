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

struct test {
    void (*setup)(void); 
    void (*teardown)(void);
    int (*test_case)(void);
};

#endif 
