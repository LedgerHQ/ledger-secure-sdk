/**
 * @file mem_alloc.c
 * @brief Dynamic memory allocation implementation
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <string.h>
#include "exceptions.h"
#include "errors.h"
#include "os_helpers.h"
#include "os_print.h"
#include "os_utils.h"
#include "mem_alloc.h"

/*********************
 *      DEFINES
 *********************/
// sizeof of header for a allocated chunk
#define ALLOC_CHUNK_HEADER_SIZE 4
// sizeof of header for a free chunk
#define FREE_CHUNK_HEADER_SIZE  8
// Alignment for payload size
#define PAYLOAD_ALIGNEMENT      4
// size of heap header
#define HEAP_HEADER_SIZE        (sizeof(heap_t) + PAYLOAD_ALIGNEMENT)

// 16 segments enable using up to 64kBytes for Heap, so way too much
#define NUM_SEG_LISTS 16

#define GET_PTR(_heap, index) \
    ((header_t *) (((uint8_t *) _heap) + PAYLOAD_ALIGNEMENT + ((index) << 3)))
#define GET_IDX(_heap, _ptr) \
    ((((uint8_t *) (_ptr)) - ((uint8_t *) _heap) - PAYLOAD_ALIGNEMENT) >> 3)
#define GET_PREV(_heap, _ptr) \
    ((header_t *) (((uint8_t *) _heap) + PAYLOAD_ALIGNEMENT + ((_ptr)->fprev << 3)))
#define GET_NEXT(_heap, _ptr) \
    ((header_t *) (((uint8_t *) _heap) + PAYLOAD_ALIGNEMENT + ((_ptr)->fnext << 3)))

/**********************
 *      TYPEDEFS
 **********************/
typedef struct header_s {
    uint16_t size : 15;
    uint16_t allocated : 1;

    uint16_t phys_prev;  ///< physical previous chunk

    uint16_t fprev;  ///< previous free chunk in the same segment
    uint16_t fnext;  ///< next free chunk in the same segment
} header_t;

// Segregated free list bucket sizes:
// 0: 16-31 bytes
// 1: 32-63 bytes
// n: 2^n-(2^(n+1)-1) bytes
// ....
typedef struct {
    void    *end;
    size_t   nb_segs;  ///< actual number of used segments
    uint16_t free_segments[NUM_SEG_LISTS];
} heap_t;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   LOCAL FUNCTIONS
 **********************/
// This function returns the index into the array segregated explicit free lists
// corresponding to the given size.
// For 2^n <= size < 2^(n+1), this function returns n.
static inline int seglist_index(heap_t *heap, size_t size)
{
    size_t seg = 32 - __builtin_clz(size);
    if (seg >= heap->nb_segs) {
        seg = heap->nb_segs - 1;
    }
    return seg;
}

// remove item from linked list
static inline void list_remove(heap_t *heap, uint16_t *first_free, uint16_t elem)
{
    header_t *elem_ptr = GET_PTR(heap, elem);
    // if was the first, set a new first
    if (*first_free == elem) {
        *first_free = elem_ptr->fnext;
        // if the current first was not alone
        if (*first_free) {
            GET_PTR(heap, *first_free)->fprev = 0;
        }
        return;
    }
    // link previous to following, if existing previous
    if (elem_ptr->fprev) {
        GET_PREV(heap, elem_ptr)->fnext = elem_ptr->fnext;
    }
    // link following to previous, if existing following
    if (elem_ptr->fnext) {
        GET_NEXT(heap, elem_ptr)->fprev = elem_ptr->fprev;
    }
}

// add item in LIFO
static inline void list_push(heap_t *heap, header_t *header)
{
    uint8_t  seg_index = seglist_index(heap, header->size);
    uint16_t first_idx = heap->free_segments[seg_index];
    // if it's already the first item, nothing to do
    if (first_idx == GET_IDX(heap, header)) {
        return;
    }
    // link this new first to following (previous first)
    header->fnext     = first_idx;
    header->fprev     = 0;
    header->allocated = 0;
    // link following (previous first) to new first
    if (first_idx != 0) {
        header_t *first = GET_PTR(heap, first_idx);
        first->fprev    = GET_IDX(heap, header);
    }
    // replace new first
    heap->free_segments[seg_index] = GET_IDX(heap, header);
}

// ensure the chunk is valid
static inline void ensure_chunk_valid(heap_t *heap, header_t *header)
{
    uint8_t *block = (uint8_t *) header;
    // ensure size is valid (multiple of pointers) and minimal size of 4 pointers)
    if ((header->size & (PAYLOAD_ALIGNEMENT - 1)) || (header->size < FREE_CHUNK_HEADER_SIZE)
        || ((block + header->size) > (uint8_t *) heap->end)) {
        PRINTF("invalid size: header->size  = %d!\n", header->size);
        THROW(EXCEPTION_CORRUPT);
    }
    //  ensure phys_prev is valid
    if (header->phys_prev != 0) {
        header_t *prev = GET_PTR(heap, header->phys_prev);
        block          = (uint8_t *) heap;
        if (((uint8_t *) prev < &block[HEAP_HEADER_SIZE]) || ((void *) prev > heap->end)) {
            PRINTF("corrupted prev_chunk\n");
            THROW(EXCEPTION_CORRUPT);
        }
        // ensure that next of prev is current
        if ((((uint8_t *) prev) + prev->size) != (uint8_t *) header) {
            PRINTF("corrupted prev_chunk\n");
            THROW(EXCEPTION_CORRUPT);
        }
    }
}

// coalesce the two given chunks
static header_t *coalesce(heap_t *heap, header_t *header, header_t *neighbour)
{
    uint16_t neighbour_idx = GET_IDX(heap, neighbour);
    ensure_chunk_valid(heap, neighbour);

    // if not allocated, coalesce
    if (!neighbour->allocated) {
        // remove this neighbour from its free list
        list_remove(
            heap, &heap->free_segments[seglist_index(heap, neighbour->size)], neighbour_idx);
        // link the current next physical chunk (if existing) to this new chunk
        header_t *next;
        if (header < neighbour) {
            next = (header_t *) (((uint8_t *) neighbour) + neighbour->size);
            if ((void *) next < heap->end) {
                next->phys_prev = GET_IDX(heap, header);
            }
            header->size += neighbour->size;
        }
        else {
            next            = (header_t *) (((uint8_t *) header) + header->size);
            next->phys_prev = neighbour_idx;
            neighbour->size += header->size;
            header = neighbour;
        }
    }
    return header;
}

static bool parse_callback(void *data, uint8_t *addr, bool allocated, size_t size)
{
    mem_stat_t *stat = (mem_stat_t *) data;

    UNUSED(addr);
    // error reached
    if (size == 0) {
        return true;
    }
    stat->nb_chunks++;
    if (allocated) {
        stat->nb_allocated++;
        stat->allocated_size += size;
    }
    else {
        stat->free_size += size;
    }
    stat->total_size += size;

    return false;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief initializes the heap for this allocator
 *
 * @param heap_start address of the heap to use
 * @param heap_size size in bytes of the heap to use
 * @return the context to use for further calls
 */
mem_ctx_t mem_init(void *heap_start, size_t heap_size)
{
    heap_t *heap = (heap_t *) heap_start;

    heap->end = ((uint8_t *) heap_start) + heap_size;

    // compute number of segments
    heap->nb_segs = 32 - __builtin_clz(heap_size);
    memset(heap->free_segments, 0, heap->nb_segs * sizeof(uint16_t));

    // initiate free chunk LIFO with the whole heap as a free chunk
    header_t *first_free  = (header_t *) (((uint8_t *) heap) + HEAP_HEADER_SIZE);
    first_free->size      = heap_size - HEAP_HEADER_SIZE;
    first_free->phys_prev = 0;
    list_push(heap, first_free);

    return (mem_ctx_t) heap_start;
}

/**
 * @brief allocates a buffer of the given size, if possible
 * @note this buffer should be free later with @ref mem_free
 *
 * @param ctx allocator context (returned by @ref mem_init())
 * @param nb_bytes size in bytes of the buffer to allocate
 * @return either a valid address if successful, or NULL if failed
 */
void *mem_alloc(mem_ctx_t ctx, size_t nb_bytes)
{
    heap_t *heap = (heap_t *) ctx;
    size_t  seg;
    // Adjust size to include header and satisfy alignment requirements, etc.
    nb_bytes += PAYLOAD_ALIGNEMENT - 1;
    nb_bytes &= ~(PAYLOAD_ALIGNEMENT - 1);

    // if nb_bytes = 8N + 4, just add 4 for header
    if (nb_bytes & PAYLOAD_ALIGNEMENT) {
        nb_bytes += ALLOC_CHUNK_HEADER_SIZE;  // for header (size +  prev_chunk)
    }
    else {
        // if nb_bytes = 8N, add 8 for header + unused footer
        nb_bytes += ALLOC_CHUNK_HEADER_SIZE + PAYLOAD_ALIGNEMENT;
    }

    // get the segment sure to be holding this size
    // if nb_bytes == 2^n , all chunks in [2^n: 2^(n+1)[ are ok
    // if nb_bytes > 2^n , all chunks in [2^(n+1): 2^(n+2)[ are ok
    seg = seglist_index(heap, nb_bytes);
    if ((nb_bytes > (size_t) (1 << seg)) && (seg < (heap->nb_segs))) {
        seg++;
    }
    // Loop through all increasing size segments
    while (seg < heap->nb_segs) {
        uint16_t chunk_idx = heap->free_segments[seg];
        // if this segment is not empty, it must contain a big-enough chunk
        if (chunk_idx != 0) {
            header_t *header = GET_PTR(heap, chunk_idx);

            //  ensure chunk is consistent
            ensure_chunk_valid(heap, header);
            // this block shall always be big enough
            if (nb_bytes > header->size) {
                // probably a big issue, maybe necessary to throw an exception
                return NULL;
            }
            uint8_t *block = (uint8_t *) header;

            // remove the block from the segregated list
            list_remove(heap, &heap->free_segments[seg], chunk_idx);
            // We could turn the excess bytes into a new free block
            // the minimum size is the size of an empty chunk
            if (header->size >= (nb_bytes + FREE_CHUNK_HEADER_SIZE)) {
                header_t *new_free = (header_t *) &block[nb_bytes];
                new_free->size     = header->size - nb_bytes;
                // link this new chunk to previous (found) one
                new_free->phys_prev = chunk_idx;
                // link the current next physical chunk to this new chunk (if existing)
                header_t *next = (header_t *) &block[header->size];
                if ((void *) next < heap->end) {
                    next->phys_prev = GET_IDX(heap, new_free);
                }
                list_push(heap, new_free);
                // change size only if new chunk is created
                header->size = nb_bytes;
            }

            // Update the chunk's header to set the allocated bit
            header->allocated = 0x1;

            // Return a pointer to the payload
            return block + ALLOC_CHUNK_HEADER_SIZE;
        }
        // not found in current segment, let's try in bigger segment
        seg++;
    }
    return NULL;
}

/**
 * @brief frees the given buffer
 *
 * @param ctx allocator context (returned by @ref mem_init())
 * @param ptr buffer previously allocated with @ref mem_alloc
 */
void mem_free(mem_ctx_t ctx, void *ptr)
{
    heap_t   *heap   = (heap_t *) ctx;
    uint8_t  *block  = ((uint8_t *) ptr) - ALLOC_CHUNK_HEADER_SIZE;
    header_t *header = (header_t *) block;

    // ensure size is consistent
    ensure_chunk_valid(heap, header);
    // if not allocated, return
    if (!header->allocated) {
        return;
    }
    // try to coalesce with adjacent physical chunks (after and before)
    // try with next physical chunk
    if ((block + header->size) < (uint8_t *) heap->end) {
        header = coalesce(heap, header, (header_t *) (block + header->size));
    }
    // try previous chunk
    if (header->phys_prev != 0) {
        header = coalesce(heap, header, GET_PTR(heap, header->phys_prev));
    }

    // free chunk and add it on top of free chunk LIFO
    list_push(heap, header);
}

/**
 * @brief parse the heap
 *
 * @param ctx allocator context (returned by @ref mem_init())
 * @param ptr buffer previously allocated with @ref mem_alloc
 * @param data context data, passed to given callback
 */
void mem_parse(mem_ctx_t ctx, mem_parse_callback_t callback, void *data)
{
    heap_t   *heap   = (heap_t *) ctx;
    header_t *header = (header_t *) (((uint8_t *) heap) + HEAP_HEADER_SIZE);
#ifdef DEBUG_FREE_SEGMENTS
    for (size_t i = 0; i < heap->nb_segs; i++) {
        if (heap->free_segments[i] != NULL) {
            header_t *head      = heap->free_segments[i];
            int       nb_elemts = 0;
            while (head) {
                nb_elemts++;
                head = head->fnext;
            }
        }
    }
#endif  // DEBUG_FREE_SEGMENTS
    while (header != NULL) {
        uint8_t *block = (uint8_t *) header;
        // break the loop if the callback returns true
        if (callback(data, block, header->allocated, header->size)) {
            return;
        }
        if (&block[header->size] < (uint8_t *) heap->end) {
            header = (header_t *) &block[header->size];
        }
        else {
            return;
        }
    }
}

/**
 * @brief function used to get statistics (nb chunks, nb allocated chunks, total size)  about the
 * heap
 *
 * @param ctx allocator context (returned by @ref mem_init())
 * @param ptr buffer previously allocated with @ref mem_alloc
 */
void mem_stat(mem_ctx_t *ctx, mem_stat_t *stat)
{
    memset(stat, 0, sizeof(mem_stat_t));
    stat->total_size = HEAP_HEADER_SIZE;
    mem_parse(ctx, parse_callback, stat);
}
