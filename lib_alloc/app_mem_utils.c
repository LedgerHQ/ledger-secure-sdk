/**
 * Dynamic allocator for an Application
 *
 * The provided API allows to hide the underlying memory allocator implementation.
 * and provide mechanisms for memory profiling (need flag HAVE_MEMORY_PROFILING).
 */

#include <stdint.h>
#include <string.h>
#include "os_helpers.h"
#include "os_print.h"
#include "mem_alloc.h"
#include "app_mem_utils.h"

static mem_ctx_t mem_utils_ctx = NULL;

#ifdef HAVE_MEMORY_PROFILING
#define MP_LOG_PREFIX "==MP "
#endif

/**
 * @brief Initializes the App heap buffer
 *
 * @param[in] heap_start address of the Application heap to use
 * @param[in] heap_size size in bytes of the heap
 * @return true if successful, false otherwise
 *
 * @note Size must be a multiple of 8, and at least 200 B, but less than 32kB
 */
bool mem_utils_init(void *heap_start, size_t heap_size)
{
    mem_utils_ctx = mem_init(heap_start, heap_size);
#ifdef HAVE_MEMORY_PROFILING
    PRINTF(MP_LOG_PREFIX "init;0x%p;%u\n", heap_start, heap_size);
#endif
    return mem_utils_ctx != NULL;
}

/**
 * @brief Internal implementation of memory allocation
 *
 * @param[in] size in bytes to allocate (must be > 0)
 * @param[in] permanent if true, the allocation is never freed (for profiling only)
 * @param[in] file source file requesting the allocation (for profiling only)
 * @param[in] line line in the source file requesting the allocation (for profiling only)
 * @return either a valid address if successful, or NULL if failed
 */
void *mem_utils_alloc(size_t size, bool permanent, const char *file, int line)
{
    UNUSED(permanent);
    UNUSED(file);
    UNUSED(line);
    void *ptr = NULL;

    ptr = mem_alloc(mem_utils_ctx, size);
#ifdef HAVE_MEMORY_PROFILING
    if (permanent) {
        PRINTF(MP_LOG_PREFIX "persist;%u;0x%p;%s:%u\n", size, ptr, file, line);
    }
    else {
        PRINTF(MP_LOG_PREFIX "alloc;%u;0x%p;%s:%u\n", size, ptr, file, line);
    }
#endif
    return ptr;
}

/**
 * @brief Internal implementation of memory reallocation
 *
 * @param[in] ptr previously allocated or NULL (equivalent to alloc)
 * @param[in] size new size in bytes to allocate. Can be 0 (equivalent to free).
 * @param[in] file source file requesting the allocation (for profiling only)
 * @param[in] line line in the source file requesting the allocation (for profiling only)
 * @return either a valid address if successful, or NULL if failed or size is 0.
 */
void *mem_utils_realloc(void *ptr, size_t size, const char *file, int line)
{
    UNUSED(file);
    UNUSED(line);
    void *new_ptr;
    new_ptr = mem_realloc(mem_utils_ctx, ptr, size);

#ifdef HAVE_MEMORY_PROFILING
    if (ptr != NULL && size == 0) {
        PRINTF(MP_LOG_PREFIX "free;0x%p;%s:%u\n", ptr, file, line);
    }
    else if (new_ptr != NULL) {
        if (ptr == NULL) {
            PRINTF(MP_LOG_PREFIX "alloc;%u;0x%p;%s:%u\n", size, new_ptr, file, line);
        }
        else if (ptr != new_ptr) {
            PRINTF(MP_LOG_PREFIX "free;0x%p;%s:%u\n", ptr, file, line);
            PRINTF(MP_LOG_PREFIX "alloc;%u;0x%p;%s:%u\n", size, new_ptr, file, line);
        }
    }
#endif
    return new_ptr;
}

/**
 * @brief Internal implementation of memory free
 *
 * @param[in] ptr previously allocated
 * @param[in] file source file requesting the allocation (for profiling only)
 * @param[in] line line in the source file requesting the allocation (for profiling only)
 */
void mem_utils_free(void *ptr, const char *file, int line)
{
    UNUSED(file);
    UNUSED(line);
    if (ptr == NULL) {
        return;
    }
#ifdef HAVE_MEMORY_PROFILING
    PRINTF(MP_LOG_PREFIX "free;0x%p;%s:%u\n", ptr, file, line);
#endif
    mem_free(mem_utils_ctx, ptr);
}

/**
 * @brief Internal implementation of memory allocation of a zero-initialized buffer
 *
 * @param[out] buffer pointer to the buffer to allocate
 * @param[in] size (in bytes) to allocate
 * @param[in] permanent if true, the allocation is never freed (for profiling only)
 * @param[in] file source file requesting the allocation (for profiling only)
 * @param[in] line line in the source file requesting the allocation (for profiling only)
 * @return true if the allocation was successful, false otherwise
 */
bool mem_utils_calloc(void **buffer, uint16_t size, bool permanent, const char *file, int line)
{
    if (size == 0) {
        // Nothing to allocate, but cleanup was done if needed
        PRINTF("Requested buffer size is zero\n");
        return true;
    }
    // Allocate the message buffer
    if ((*buffer = mem_utils_alloc(size, permanent, file, line)) == NULL) {
        PRINTF("Memory allocation failed for buffer of size %u\n", size);
        return false;
    }
    explicit_bzero(*buffer, size);
    return true;
}

/**
 * @brief Internal implementation of safe free with nullification
 *
 * @param[in,out] buffer Pointer to the buffer to deallocate
 * @param[in] file source file requesting the allocation (for profiling only)
 * @param[in] line line in the source file requesting the allocation (for profiling only)
 */
void mem_utils_free_and_null(void **buffer, const char *file, int line)
{
    if (*buffer != NULL) {
        mem_utils_free(*buffer, file, line);
        *buffer = NULL;
    }
}

/**
 * @brief Internal implementation of string duplication
 *
 * @param[in] src string to duplicate
 * @param[in] file source file requesting the allocation (for profiling only)
 * @param[in] line line in the source file requesting the allocation (for profiling only)
 * @return either a valid address if successful, or NULL if failed
 */
char *mem_utils_strdup(const char *src, const char *file, int line)
{
    char  *dst    = NULL;
    size_t length = strlen(src) + 1;

    if ((dst = mem_utils_alloc(length, false, file, line)) != NULL) {
        memcpy(dst, src, length);
    }
    return dst;
}
