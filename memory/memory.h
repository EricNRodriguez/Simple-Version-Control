#ifndef ASSIGNMENT_2_SVC_MEMORY_H
#define ASSIGNMENT_2_SVC_MEMORY_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/** @brief Wraps malloc, calls perror and exits on error.
 *
 *  If malloc returns null, perror is called and program exits with status 2.
 *
 *  @param size : number of bytes to be allocated.
 *  @return address of allocated memory.
 */
void *safe_malloc(size_t size);

/** @brief Wraps realloc, calls perror and exits on error.
 *
 *  If realloc returns null, perror is called and program exits with status 2.
 *
 *  @param old : address of memory to be reallocated.
 *  @param size : number of bytes to be allocated.
 *  @return address of reallocated memory.
 */
void *safe_realloc(void *old, size_t size);

/** @brief Copies string.
 *
 *  Copies null terminated string to new, dynamicaly allocated address.
 *  Returned address MUST be released.
 *
 *  @param string : null terminated string.
 *  @return copied string.
 */
char *copy_string(char *string);

#endif //ASSIGNMENT_2_SVC_MEMORY_H
