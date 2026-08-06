#pragma once
/**
 * @file fuzz_mutator.h
 * @brief Prefix-aware LibFuzzer custom mutator.
 *
 * A fuzzer input is @c [ Absolution prefix | harness input ] (see fuzz_defs.h).
 * LibFuzzer's default mutator treats that as a flat byte array, which mostly
 * produces incoherent global state; @ref fuzz_custom_mutator() splits the two
 * and treats them differently:
 *
 * - the harness input is mutated freely by @c LLVMFuzzerMutate(),
 * - the sampled prefix is perturbed in small windows so restored state stays
 *   coherent,
 * - the harness control bytes (lane, command) are mutated on their own, so the
 *   fuzzer can deliberately switch lane or command instead of waiting to hit
 *   those bytes by chance.
 *
 * fuzz_harness.h forwards @c LLVMFuzzerCustomMutator() here by default; the only
 * layout fact needed is the prefix *size*.
 *
 * An app that wants field-aware tweaks on top of the default defines
 * @c FUZZ_APP_POST_MUTATE; one with its own input grammar defines
 * @c FUZZ_APP_CUSTOM_MUTATOR and builds on @ref fuzz_mutate_input_with(). For
 * TLV-framed payloads see tlv_mutator.h.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "fuzz_defs.h"

extern size_t LLVMFuzzerMutate(uint8_t *data, size_t size, size_t max_size);

enum {
    FUZZ_MUT_INITIAL_INPUT_SIZE = 256,  ///< Size of the input synthesised for an empty seed.
    FUZZ_MUT_PREFIX_WINDOW_MAX  = 32,   ///< Largest prefix window mutated at once.
    /* Structured inputs: 60% mutate the harness input, 30% the control bytes,
     * 10% a prefix window. Raw inputs mostly ignore the prefix. */
    FUZZ_MUT_INPUT_CUTOFF    = 60,  ///< Dice below this: mutate the harness input.
    FUZZ_MUT_CTRL_CUTOFF     = 90,  ///< Dice below this: mutate the control bytes.
    FUZZ_MUT_RAW_PREFIX_MASK = 3,   ///< Raw lane touches the prefix 1 time in 4.
};

/**
 * @brief Where Absolution's sampled state ends.
 *
 * Read from @ref fuzz_absolution_prefix_size, which the build generates and links
 * in. Never zero: EmitPrefixSize.cmake fails the build instead, since zero would
 * degrade this to flat-byte mutation over structured state.
 */
static inline size_t fuzz_prefix_size(void)
{
    return fuzz_absolution_prefix_size;
}

/** @brief Mutate one bounded window of the sampled prefix, keeping state coherent. */
static inline void fuzz_mutate_prefix_window(uint8_t *data, size_t prefix_size, unsigned int pick)
{
    if (prefix_size == 0) {
        return;
    }

    size_t start = pick % prefix_size;
    size_t span  = prefix_size - start;
    if (span > FUZZ_MUT_PREFIX_WINDOW_MAX) {
        span = FUZZ_MUT_PREFIX_WINDOW_MAX;
    }

    (void) LLVMFuzzerMutate(data + start, span, span);
}

/**
 * @brief Mutate only the lane/command bytes.
 *
 * Lets a single mutation flip the input to another lane or another APDU command.
 */
static inline void fuzz_mutate_ctrl(uint8_t *input)
{
    (void) LLVMFuzzerMutate(input, FUZZ_CTRL_LEN, FUZZ_CTRL_LEN);
}

/** @brief Give an empty input a zero-filled prefix and a mutable tail to start from. */
static inline size_t fuzz_bootstrap_input(uint8_t *data,
                                          size_t   size,
                                          size_t   prefix_size,
                                          size_t   max_size)
{
    if (size != 0) {
        return size;
    }

    size = prefix_size + FUZZ_MUT_INITIAL_INPUT_SIZE;
    if (size > max_size) {
        size = max_size;
    }
    memset(data, 0, size);
    return size;
}

/** @brief The framework's @c LLVMFuzzerCustomMutator() body. */
static inline size_t fuzz_custom_mutator(uint8_t     *data,
                                         size_t       size,
                                         size_t       max_size,
                                         unsigned int seed)
{
    const size_t prefix_size = fuzz_prefix_size();

    /* No usable prefix size, or no room for a harness input after it. */
    if (prefix_size == 0 || prefix_size + FUZZ_CTRL_LEN >= max_size) {
        return LLVMFuzzerMutate(data, size, max_size);
    }

    size = fuzz_bootstrap_input(data, size, prefix_size, max_size);

    if (size <= prefix_size + FUZZ_CTRL_LEN) {
        return LLVMFuzzerMutate(data, size, max_size);
    }

    uint8_t     *input      = data + prefix_size;
    size_t       input_size = size - prefix_size;
    unsigned int dice       = seed % 100;

    if (input[0] > FUZZ_STRUCTURED_LANE_THRESHOLD) {
        if (dice < FUZZ_MUT_INPUT_CUTOFF) {
            input_size = LLVMFuzzerMutate(input, input_size, max_size - prefix_size);
        }
        else if (dice < FUZZ_MUT_CTRL_CUTOFF) {
            fuzz_mutate_ctrl(input);
        }
        else {
            fuzz_mutate_prefix_window(data, prefix_size, seed >> 8);
        }
    }
    else {
        if ((seed & FUZZ_MUT_RAW_PREFIX_MASK) == 0) {
            fuzz_mutate_prefix_window(data, prefix_size, seed >> 2);
        }
        input_size = LLVMFuzzerMutate(input, input_size, max_size - prefix_size);
    }

#ifdef FUZZ_APP_POST_MUTATE
    /* Hook for app-specific, field-aware mutation of the harness input. */
    FUZZ_APP_POST_MUTATE(input, input_size, max_size - prefix_size, seed);
#endif

    return prefix_size + input_size;
}

/** @brief Mutates a harness input in place, returning its new size. */
typedef size_t (*fuzz_input_mutator_fn)(uint8_t     *input,
                                        size_t       size,
                                        size_t       max_size,
                                        unsigned int seed);

/**
 * @brief Run a grammar-aware mutator over the harness input only, leaving the
 *        sampled prefix untouched.
 *
 * Harnesses with their own input grammar (TLV, for example) use this instead of
 * repeating the prefix arithmetic themselves.
 */
static inline size_t fuzz_mutate_input_with(uint8_t              *data,
                                            size_t                size,
                                            size_t                max_size,
                                            unsigned int          seed,
                                            fuzz_input_mutator_fn mutate_input)
{
    const size_t prefix_size = fuzz_prefix_size();

    if (prefix_size == 0 || prefix_size >= max_size || size <= prefix_size) {
        return fuzz_custom_mutator(data, size, max_size, seed);
    }

    size_t input_size
        = mutate_input(data + prefix_size, size - prefix_size, max_size - prefix_size, seed);

    return prefix_size + input_size;
}
