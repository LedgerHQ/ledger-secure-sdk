/**
 * @file mem_alloc.h
 * @brief Dynamic memory allocation API
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**********************
 *      TYPEDEFS
 **********************/
/**
 * @brief type shared externally
 *
 */
typedef void *mem_ctx_t;

/**
 * @brief function called for each chunk found in the heap.
 *
 * @param data context data, passed as argument of @ref mem_parse
 * @param addr address of the chunk (as returned by @ref mem_alloc) if allocated
 * @param allocated if true, means the chunk is allocated
 * @param size size of the chunk, in bytes (including header & alignment bytes)
 *
 * @return shall return true to break parsing
 *
 */
typedef bool (*mem_parse_callback_t)(void *data, uint8_t *addr, bool allocated, size_t size);

/**
 * @brief this structure, filled by @ref mem_stat
 *
 */
typedef struct {
    size_t total_size;        ///< total size of the heap (including allocator internal data)
    size_t free_size;         ///< nb bytes free in the heap (be careful, it doesn't mean it can be
                              ///< allocated in a single chunk)
    size_t   allocated_size;  ///< nb bytes allocated in the heap (including headers)
    uint32_t nb_chunks;       ///< total number of chunks
    uint32_t nb_allocated;    ///< number of allocated chunks
} mem_stat_t;

/**********************
 *      GLOBAL PROTOTYPES
 **********************/

mem_ctx_t mem_init(void *heap_start, size_t heap_size);
void     *mem_alloc(mem_ctx_t ctx, size_t nb_bytes);
void     *mem_realloc(mem_ctx_t ctx, void *ptr, size_t size);
void      mem_free(mem_ctx_t ctx, void *ptr);
void      mem_parse(mem_ctx_t ctx, mem_parse_callback_t callback, void *dat);
void      mem_stat(mem_ctx_t *ctx, mem_stat_t *stat);

#ifdef __cplusplus
} /* extern "C" */
#endif
