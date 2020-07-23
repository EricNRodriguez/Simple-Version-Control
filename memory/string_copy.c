#include "memory.h"

char *copy_string(char *string) {
    if (!string) {
        return NULL;
    }

    // allocate new memory and copy chars
    char *str_cpy = safe_malloc(strlen(string) + 1);
    strcpy(str_cpy, string);

    return str_cpy;
}