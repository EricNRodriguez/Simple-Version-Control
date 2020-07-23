#include "memory.h"

void *safe_malloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        perror("malloc failed\n");
        exit(2);
    }
    return p;
}

void *safe_realloc(void *old, size_t size) {
    void *p = realloc(old, size);
    if (!p) {
        perror("realloc failed\n");
        exit(2);
    }
    return p;
}