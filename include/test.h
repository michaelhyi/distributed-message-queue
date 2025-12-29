#ifndef TEST_H
#define TEST_H

#define assert(expr)                                                           \
    if (!(expr)) {                                                             \
        fprintf(stderr, "Assertion failed at %s:%d with errno: %s\n",          \
                __FILE__, __LINE__, strerror(errno));                          \
        return -1;                                                             \
    }

struct test {
    void (*setup)(void); 
    void (*teardown)(void);
    int (*test_case)(void);
};

#endif 
