#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef HAVE_MEMORY_PROFILING
#define ALLOC_FILE __FILE__
#define ALLOC_LINE __LINE__
#else
#define ALLOC_FILE NULL
#define ALLOC_LINE 0
#endif

// Utility wrappers to handle file and line info automatically (for profiling)
#define APP_MEM_ALLOC(size)          mem_utils_alloc(size, false, ALLOC_FILE, ALLOC_LINE)
#define APP_MEM_FREE(ptr)            mem_utils_free(ptr, ALLOC_FILE, ALLOC_LINE)
#define APP_MEM_FREE_AND_NULL(ptr)   mem_utils_free_and_null(ptr, ALLOC_FILE, ALLOC_LINE)
#define APP_MEM_STRDUP(ptr)          mem_utils_strdup(ptr, ALLOC_FILE, ALLOC_LINE)
#define APP_MEM_CALLOC(ptr, size)    mem_utils_calloc(ptr, size, false, ALLOC_FILE, ALLOC_LINE)
#define APP_MEM_PERMANENT(ptr, size) mem_utils_calloc(ptr, size, true, ALLOC_FILE, ALLOC_LINE)

bool  mem_utils_init(void *heap_start, size_t heap_size);
void *mem_utils_alloc(size_t size, bool permanent, const char *file, int line);
void  mem_utils_free(void *ptr, const char *file, int line);
void  mem_utils_free_and_null(void **buffer, const char *file, int line);
char *mem_utils_strdup(const char *s, const char *file, int line);
bool  mem_utils_calloc(void **buffer, uint16_t size, bool permanent, const char *file, int line);
