/**
 * @file fuzz_mutator.c
 * @brief Prefix-aware LibFuzzer custom mutator; see fuzz_mutator.h.
 */

#include "fuzz_mutator.h"

/** Size of the input synthesised for an empty seed. */
#define FUZZ_MUT_INITIAL_INPUT_SIZE 256
/** Largest prefix window mutated at once. */
#define FUZZ_MUT_PREFIX_WINDOW_MAX  32
/* Structured inputs: 60% mutate the harness input, 30% the control bytes,
 * 10% a prefix window. Raw inputs mostly ignore the prefix. */
/** Dice below this: mutate the harness input. */
#define FUZZ_MUT_INPUT_CUTOFF       60
/** Dice below this: mutate the control bytes. */
#define FUZZ_MUT_CTRL_CUTOFF        90
/** Raw lane touches the prefix 1 time in 4. */
#define FUZZ_MUT_RAW_PREFIX_MASK    3

/**
 * @brief Where Absolution's sampled state ends.
 *
 * Read from @ref fuzz_absolution_prefix_size, which the build generates and links
 * in. Never zero: EmitPrefixSize.cmake fails the build instead, since zero would
 * degrade this to flat-byte mutation over structured state.
 */
size_t fuzz_prefix_size(void)
{
    return fuzz_absolution_prefix_size;
}

/** @brief Mutate one bounded window of the sampled prefix, keeping state coherent. */
static void fuzz_mutate_prefix_window(uint8_t *data, size_t prefix_size, unsigned int pick)
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
static void fuzz_mutate_ctrl(uint8_t *input)
{
    (void) LLVMFuzzerMutate(input, FUZZ_CTRL_LEN, FUZZ_CTRL_LEN);
}

/** @brief Give an empty input a zero-filled prefix and a mutable tail to start from. */
static size_t fuzz_bootstrap_input(uint8_t *data, size_t size, size_t prefix_size, size_t max_size)
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
size_t fuzz_custom_mutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
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

    return prefix_size + input_size;
}

size_t fuzz_mutate_input_with(uint8_t              *data,
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
