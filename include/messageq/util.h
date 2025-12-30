#ifndef UTILS_H
#define UTILS_H

#ifdef DEBUG
#include <stdio.h>
#define dprintf(...) printf(__VA_ARGS__);
#else
#define dprintf(...)
#endif

#define arrlen(arr) (int)(sizeof arr / sizeof arr[0])

#endif
