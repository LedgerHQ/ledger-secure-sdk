#pragma once
/*
 * Prefix-aware custom mutator.
 *
 * Required before inclusion:
 * - FUZZ_PREFIX_SIZE_FALLBACK
 * - FUZZ_CTRL_OFF
 * - FUZZ_CTRL_LEN
 * - fuzz_lane_is_structured(data, prefix_size)
 *
 * Optional:
 * - FUZZ_APP_DATA_OFF / FUZZ_APP_DATA_LEN
 * - FUZZ_INJECT_TOKEN(region, region_len, seed)
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef FUZZ_APP_DATA_OFF
#define FUZZ_APP_DATA_OFF 0
#endif
#ifndef FUZZ_APP_DATA_LEN
#define FUZZ_APP_DATA_LEN 0
#endif

extern size_t       LLVMFuzzerMutate(uint8_t *data, size_t size, size_t max_size);
extern const size_t absolution_globals_size __attribute__((weak));

enum {
    FUZZ_MUT_INITIAL_POST_PREFIX_SIZE = 256,
    FUZZ_MUT_PREFIX_WINDOW_MAX        = 32,
    FUZZ_MUT_APP_DATA_WINDOW_MAX      = 64,
    FUZZ_MUT_STRUCTURED_CTRL_CUTOFF   = 90,
    FUZZ_MUT_RAW_PREFIX_MASK          = 3,
    FUZZ_MUT_TOKEN_INJECT_MASK        = 1,
};

#if FUZZ_APP_DATA_LEN > 0
enum {
    FUZZ_MUT_STRUCTURED_APP_DATA_CUTOFF    = 40,
    FUZZ_MUT_STRUCTURED_POST_PREFIX_CUTOFF = 70,
};
#else
enum {
    FUZZ_MUT_STRUCTURED_POST_PREFIX_CUTOFF = 60,
};
#endif

static size_t fuzz_prefix_size(void)
{
    size_t prefix_size = FUZZ_PREFIX_SIZE_FALLBACK;

    if (&absolution_globals_size != NULL && absolution_globals_size != 0) {
        prefix_size = absolution_globals_size;
    }
    return prefix_size;
}

static int fuzz_split_is_valid(size_t prefix_size, size_t max_size)
{
    if (prefix_size == 0 || prefix_size >= max_size) {
        return 0;
    }

    if (FUZZ_CTRL_OFF + FUZZ_CTRL_LEN > prefix_size) {
        return 0;
    }

#if FUZZ_APP_DATA_LEN > 0
    if (FUZZ_APP_DATA_OFF + FUZZ_APP_DATA_LEN > prefix_size) {
        return 0;
    }
#endif
    return 1;
}

static size_t fuzz_bootstrap_input(uint8_t *data, size_t size, size_t prefix_size, size_t max_size)
{
    if (size == 0) {
        if (prefix_size + FUZZ_MUT_INITIAL_POST_PREFIX_SIZE <= max_size) {
            size = prefix_size + FUZZ_MUT_INITIAL_POST_PREFIX_SIZE;
        }
        else {
            size = max_size;
        }
        memset(data, 0, size);
    }
    return size;
}

static void fuzz_mutate_window(uint8_t *data, size_t data_len, size_t max_span, unsigned int pick)
{
    if (data_len == 0) {
        return;
    }

    size_t start = pick % data_len;
    size_t span  = data_len - start;
    if (span > max_span) {
        span = max_span;
    }

    (void) LLVMFuzzerMutate(data + start, span, span);
}

static void fuzz_mutate_prefix_window(uint8_t *data, size_t prefix_size, unsigned int pick)
{
    fuzz_mutate_window(data, prefix_size, FUZZ_MUT_PREFIX_WINDOW_MAX, pick);
}

static void fuzz_mutate_ctrl(uint8_t *data)
{
    (void) LLVMFuzzerMutate(data + FUZZ_CTRL_OFF, FUZZ_CTRL_LEN, FUZZ_CTRL_LEN);
}

static size_t fuzz_mutate_post_prefix(uint8_t *data, size_t size, size_t max_size)
{
    return LLVMFuzzerMutate(data, size, max_size);
}

static int fuzz_should_inject_token(unsigned int seed)
{
    return ((seed >> 4) & FUZZ_MUT_TOKEN_INJECT_MASK) == 0;
}

#if FUZZ_APP_DATA_LEN > 0
static void fuzz_mutate_app_data(uint8_t *data, unsigned int seed)
{
    fuzz_mutate_window(
        data + FUZZ_APP_DATA_OFF, FUZZ_APP_DATA_LEN, FUZZ_MUT_APP_DATA_WINDOW_MAX, seed >> 8);
}

#ifdef FUZZ_INJECT_TOKEN
static void fuzz_maybe_inject_structured_token(uint8_t *data, unsigned int seed)
{
    if (fuzz_should_inject_token(seed)) {
        FUZZ_INJECT_TOKEN(data + FUZZ_APP_DATA_OFF, FUZZ_APP_DATA_LEN, seed >> 16);
    }
}
#endif
#endif

#ifdef FUZZ_INJECT_TOKEN
static void fuzz_maybe_inject_raw_token(uint8_t *data, size_t post_prefix_size, unsigned int seed)
{
    if (post_prefix_size > 0 && fuzz_should_inject_token(seed)) {
        FUZZ_INJECT_TOKEN(data, post_prefix_size, seed >> 16);
    }
}
#endif

static size_t fuzz_mutate_structured(uint8_t     *data,
                                     size_t       prefix_size,
                                     size_t       post_prefix_size,
                                     size_t       max_size,
                                     unsigned int seed)
{
    unsigned int dice = seed % 100;

#if FUZZ_APP_DATA_LEN > 0
    if (dice < FUZZ_MUT_STRUCTURED_APP_DATA_CUTOFF) {
        fuzz_mutate_app_data(data, seed);
    }
    else if (dice < FUZZ_MUT_STRUCTURED_POST_PREFIX_CUTOFF) {
#else
    if (dice < FUZZ_MUT_STRUCTURED_POST_PREFIX_CUTOFF) {
#endif
        post_prefix_size
            = fuzz_mutate_post_prefix(data + prefix_size, post_prefix_size, max_size - prefix_size);
    }
    else if (dice < FUZZ_MUT_STRUCTURED_CTRL_CUTOFF) {
        fuzz_mutate_ctrl(data);
    }
    else {
        fuzz_mutate_prefix_window(data, prefix_size, seed >> 8);
    }

#if FUZZ_APP_DATA_LEN > 0
#ifdef FUZZ_INJECT_TOKEN
    fuzz_maybe_inject_structured_token(data, seed);
#endif
#endif

    return post_prefix_size;
}

static size_t fuzz_mutate_raw(uint8_t     *data,
                              size_t       prefix_size,
                              size_t       post_prefix_size,
                              size_t       max_size,
                              unsigned int seed)
{
    if ((seed & FUZZ_MUT_RAW_PREFIX_MASK) == 0) {
        fuzz_mutate_prefix_window(data, prefix_size, seed >> 2);
    }

    post_prefix_size
        = fuzz_mutate_post_prefix(data + prefix_size, post_prefix_size, max_size - prefix_size);

#ifdef FUZZ_INJECT_TOKEN
    fuzz_maybe_inject_raw_token(data + prefix_size, post_prefix_size, seed);
#endif

    return post_prefix_size;
}

static size_t fuzz_mutate_with_split(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    size_t prefix_size = fuzz_prefix_size();
    size_t post_prefix_size;

    if (!fuzz_split_is_valid(prefix_size, max_size)) {
        return LLVMFuzzerMutate(data, size, max_size);
    }

    size = fuzz_bootstrap_input(data, size, prefix_size, max_size);

    if (size <= prefix_size) {
        return LLVMFuzzerMutate(data, size, max_size);
    }

    post_prefix_size = size - prefix_size;

    if (fuzz_lane_is_structured(data, prefix_size)) {
        post_prefix_size
            = fuzz_mutate_structured(data, prefix_size, post_prefix_size, max_size, seed);
    }
    else {
        post_prefix_size = fuzz_mutate_raw(data, prefix_size, post_prefix_size, max_size, seed);
    }

    return prefix_size + post_prefix_size;
}

static size_t fuzz_custom_mutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    return fuzz_mutate_with_split(data, size, max_size, seed);
}
