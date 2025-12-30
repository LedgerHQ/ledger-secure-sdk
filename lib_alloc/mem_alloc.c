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
#include "os_math.h"
#include "os_print.h"
#include "os_utils.h"
#include "mem_alloc.h"

/*********************
 *      DEFINES
 *********************/
// Alignment for payload data size
#define PAYLOAD_ALIGNEMENT      4
// Alignment for returned payload
#define PAYLOAD_DATA_ALIGNEMENT 8
// sizeof of header for a allocated chunk
#define ALLOC_CHUNK_HEADER_SIZE 4
// sizeof of header for a free chunk
#define FREE_CHUNK_HEADER_SIZE  8
// size of heap header
#define HEAP_HEADER_SIZE \
    ((sizeof(heap_t) + PAYLOAD_DATA_ALIGNEMENT - 1) & ~(PAYLOAD_DATA_ALIGNEMENT - 1))

// number of 2^N segments aggregated as linear
#define NB_LINEAR_SEGMENTS 5
// 10 segments (the first one is linear) enable using up to ~32k bytes chunks, which is more than
// enough
/* Segregated free list bucket sizes:
- 0: 1-63 bytes
- 1: 64-127 bytes
- 2: 128-255 bytes
- 4: 512-1023 bytes
- 6: 2048-4095 bytes
- 7: 4096-8191 bytes
- n: 2^(n+5)-(2^(n+6)-1) bytes
- 9: 16384-32767 bytes
*/
#define NB_MAX_SEGMENTS    10
// 4 sub-segments to have a better segregation of each segments
#define NB_SUB_SEGMENTS    4

// index is basically the offset in u64 from beginning of full heap buffer
#define GET_PTR(_heap, index) ((header_t *) (((uint8_t *) _heap) + ((index) << 3)))
#define GET_IDX(_heap, _ptr)  ((((uint8_t *) (_ptr)) - ((uint8_t *) _heap)) >> 3)
#define GET_PREV(_heap, _ptr) ((header_t *) (((uint8_t *) _heap) + ((_ptr)->fprev << 3)))
#define GET_NEXT(_heap, _ptr) ((header_t *) (((uint8_t *) _heap) + ((_ptr)->fnext << 3)))

#define GET_SEGMENT(_size) MAX(NB_LINEAR_SEGMENTS, (31 - __builtin_clz(size)))

/**********************
 *      TYPEDEFS
 **********************/
typedef struct header_s {
    uint16_t size : 15;  // the biggest size is 0x7FFF = 0x32767
    uint16_t allocated : 1;

    uint16_t phys_prev;  ///< physical previous chunk

    uint16_t fprev;  ///< previous free chunk in the same segment
    uint16_t fnext;  ///< next free chunk in the same segment
} header_t;

typedef struct {
    void    *end;      ///< end of headp buffer, for consistency check
    size_t   nb_segs;  ///< actual number of used segments
    uint16_t free_segments[NB_MAX_SEGMENTS * NB_SUB_SEGMENTS];
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
static inline size_t align_alloc_size(size_t size)
{
    // Add alignment - 1 to prepare for rounding up if needed
    // This trick ensures already aligned sizes are not changed
    size += PAYLOAD_ALIGNEMENT - 1;
    // Mask off lower bits to align size down to nearest alignment boundary
    size &= ~(PAYLOAD_ALIGNEMENT - 1);
    // Either add 4 bytes (if size is 8N+4) or 8 bytes (if size is 8N) so the full
    // 8-byte header structure fits while preserving alignment.
    size += (size & PAYLOAD_ALIGNEMENT) ? ALLOC_CHUNK_HEADER_SIZE
                                        : ALLOC_CHUNK_HEADER_SIZE + PAYLOAD_ALIGNEMENT;
    return size;
}

// This function returns the index into the array segregated explicit free lists
// corresponding to the given size.
// For 2^n <= size < 2^(n+1), this function returns n.
static inline int seglist_index(heap_t *heap, size_t size)
{
    size_t  seg;
    uint8_t sub_segment;

    seg = GET_SEGMENT(size);
    if (seg == NB_LINEAR_SEGMENTS) {
        // for the aggregated segment, the sub-segmant size is 1/NB_SUB_SEGMENTS of the size of the
        // segment
        sub_segment = (size >> (seg - 1)) & 0x3;
    }
    else {
        // sub segment size is 1/(NB_SUB_SEGMENTS*2) of the size of the segment
        sub_segment = (size >> (seg - 2)) & 0x3;
    }
    // from size in [0 : 63[, segment is 0 to 5 but is forced to 0
    // from size in [2^6 : 2^13[, segment is [6 : 12] but is forced to [1 : 7]
    seg -= NB_LINEAR_SEGMENTS;
    // check consistency
    if (seg >= heap->nb_segs) {
        return -1;
    }
    return seg * NB_SUB_SEGMENTS + sub_segment;
}

// This function returns the index into the array segregated explicit free lists
// corresponding to the given size, being sure to be big enough (so rounded to next sub-segment).
static inline int seglist_index_up(heap_t *heap, size_t size)
{
    size_t  seg;
    uint8_t sub_segment;
    size_t  sub_segment_size;

    seg = GET_SEGMENT(size);
    if (seg == NB_LINEAR_SEGMENTS) {
        // for the aggregated segment, the sub-segmant size is 1/NB_SUB_SEGMENTS of the size of the
        // segment
        sub_segment_size = (1 << (seg - 1));
    }
    else {
        // sub segment size is 1/(NB_SUB_SEGMENTS*2) of the size of the segment
        sub_segment_size = (1 << (seg - 2));
    }

    // round size to next sub-segment
    size += sub_segment_size - 1;
    size &= ~(sub_segment_size - 1);

    seg = GET_SEGMENT(size);
    if (seg == NB_LINEAR_SEGMENTS) {
        // for the aggregated segment, the sub-segmant size is 1/NB_SUB_SEGMENTS of the size of the
        // segment
        sub_segment = (size >> (seg - 1)) & 0x3;
    }
    else {
        // sub segment size is 1/(NB_SUB_SEGMENTS*2) of the size of the segment
        sub_segment = (size >> (seg - 2)) & 0x3;
    }

    // from size in [0 : 63[, segment is 0 to 5 but is forced to 0
    // from size in [2^6 : 2^13[, segment is [6 : 12] but is forced to [1 : 7]
    seg -= NB_LINEAR_SEGMENTS;

    // check consistency
    if (seg >= heap->nb_segs) {
        return -1;
    }
    return seg * NB_SUB_SEGMENTS + sub_segment;
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
    int      seg_index = seglist_index(heap, header->size);
    uint16_t first_idx = heap->free_segments[seg_index];
    if (seg_index < 0) {
        return;
    }
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
            // clean-up neighbour to avoid leaking
            memset(neighbour, 0, sizeof(*neighbour));
        }
        else {
            next = (header_t *) (((uint8_t *) header) + header->size);
            // ensure next is valid (inside the  heap buffer) before linking it
            if ((void *) next < heap->end) {
                next->phys_prev = neighbour_idx;
            }
            neighbour->size += header->size;
            // clean-up header to avoid leaking
            memset(header, 0, sizeof(*header));
            // header points now to neighbour
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
 * @param heap_size size in bytes of the heap to use. It must be a multiple of 8, and at least 200
 * bytes but less than 32kBytes (max is exactly 32856 bytes)
 * @return the context to use for further calls, or NULL if failling
 */
mem_ctx_t mem_init(void *heap_start, size_t heap_size)
{
    heap_t *heap = (heap_t *) heap_start;

    // heap_start must not be NULL
    if (heap_start == NULL) {
        return NULL;
    }

    // size must be a multiple of 8, and at least 200 bytes
    if ((heap_size & (FREE_CHUNK_HEADER_SIZE - 1)) || (heap_size < 200)
        || (heap_size > (0x7FF8 + HEAP_HEADER_SIZE))) {
        return NULL;
    }

    heap->end = ((uint8_t *) heap_start) + heap_size;

    // compute number of segments
    heap->nb_segs = 31 - __builtin_clz(heap_size - HEAP_HEADER_SIZE) - NB_LINEAR_SEGMENTS + 1;
    if (heap->nb_segs > NB_MAX_SEGMENTS) {
        return NULL;
    }
    memset(heap->free_segments, 0, heap->nb_segs * NB_SUB_SEGMENTS * sizeof(uint16_t));

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
 * @param nb_bytes size in bytes of the buffer to allocate (must be > 0)
 * @return either a valid address if successful, or NULL if failed (no more memory or invalid
 * nb_bytes)
 */
void *mem_alloc(mem_ctx_t ctx, size_t nb_bytes)
{
    heap_t *heap = (heap_t *) ctx;
    int     seg;

    // nb_bytes must be > 0
    if (nb_bytes == 0) {
        return NULL;
    }

    // Adjust size to include header and satisfy alignment requirements, etc.
    nb_bytes = align_alloc_size(nb_bytes);

    // get the segment sure to be holding this size
    // if nb_bytes == 2^n , all chunks in [2^n: 2^(n+1)[ are ok
    // if nb_bytes > 2^n , all chunks in [2^(n+1): 2^(n+2)[ are ok
    seg = seglist_index_up(heap, nb_bytes);
    if (seg < 0) {
        return NULL;
    }
    // Loop through all increasing size segments
    while ((size_t) seg < (heap->nb_segs * NB_SUB_SEGMENTS)) {
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

            // clean-up previous empty header info
            header->fnext = header->fprev = 0;

            // Return a pointer to the payload
            return block + ALLOC_CHUNK_HEADER_SIZE;
        }
        // not found in current segment, let's try in bigger segment
        seg++;
    }
    return NULL;
}

/**
 * @brief Reallocates a buffer to a new size
 * @note The new size can be either smaller or bigger than the original one.
 * The content is preserved up to the minimum of the old and new sizes.
 * If ptr is NULL, this function behaves like @ref mem_alloc.
 * If size is 0, this function behaves like @ref mem_free and returns NULL.
 *
 * @param ctx allocator context (returned by @ref mem_init())
 * @param ptr buffer previously allocated with @ref mem_alloc
 * @param size new requested size in bytes
 * @return address of the reallocated buffer (may be the same or different), or NULL on failure
 */
void *mem_realloc(mem_ctx_t ctx, void *ptr, size_t size)
{
    heap_t *heap = (heap_t *) ctx;

    // Use mem_alloc if original pointer is NULL
    if (ptr == NULL) {
        return mem_alloc(ctx, size);
    }

    // Free the original block if new size is zero
    if (size == 0) {
        mem_free(ctx, ptr);
        return NULL;
    }

    // Get the header of the current block
    uint8_t  *block  = ((uint8_t *) ptr) - ALLOC_CHUNK_HEADER_SIZE;
    header_t *header = (header_t *) block;

    // Ensure this is a valid chunk before we try to read its size.
    ensure_chunk_valid(heap, header);

    // Save the size of the old block
    size_t old_block_size = header->size;
    // Save the size of the data portion of the old block
    size_t old_payload_size = old_block_size - ALLOC_CHUNK_HEADER_SIZE;
    // Adjust size of the new block to include header and satisfy alignment requirements, etc.
    size_t new_block_size = align_alloc_size(size);

    // If the old block is already big enough, return the same pointer
    if (old_payload_size >= size) {
        // Shrink optimization : split the original block in two
        // if remaining size is enough for a free chunk
        size_t remainder = old_block_size - new_block_size;
        if (remainder >= FREE_CHUNK_HEADER_SIZE) {
            // Save pointer to next physical chunk
            header_t *next = (header_t *) &block[old_block_size];
            // Create new free chunk in the remaining space,
            // located just after the resized block
            header_t *new_free = (header_t *) &block[new_block_size];
            new_free->size     = remainder;
            // The new chunk predecessor is the resized chunk (still the same)
            new_free->phys_prev = GET_IDX(heap, header);
            // Link the current next physical chunk to this new chunk (if existing)
            if ((void *) next < heap->end) {
                next->phys_prev = GET_IDX(heap, new_free);
            }
            list_push(heap, new_free);
            // Adjust the size of the allocated block
            header->size = new_block_size;
        }

        // Return the original block pointer as it is still valid
        return ptr;
    }

    // Allocate new block
    void *new_ptr = mem_alloc(ctx, size);
    if (new_ptr == NULL) {
        return NULL;
    }

    // We copy only the amount of data that existed in the OLD block.
    // Since we know old_payload_size < size, we copy old_payload_size.
    // This guarantees we never read past the end of the old allocated buffer.
    memcpy(new_ptr, ptr, old_payload_size);

    // Free the old block
    mem_free(ctx, ptr);

    // Return the new block pointer
    return new_ptr;
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
