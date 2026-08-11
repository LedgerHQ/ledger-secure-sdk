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
 * An app with its own input grammar defines @c FUZZ_APP_CUSTOM_MUTATOR and
 * builds on @ref fuzz_mutate_input_with(). For TLV-framed payloads see
 * tlv_mutator.h.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "fuzz_defs.h"

extern size_t LLVMFuzzerMutate(uint8_t *data, size_t size, size_t max_size);

/** @brief Where Absolution's sampled state ends (see cmake/EmitPrefixSize.cmake). */
size_t fuzz_prefix_size(void);

/** @brief The framework's @c LLVMFuzzerCustomMutator() body. */
size_t fuzz_custom_mutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed);

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
size_t fuzz_mutate_input_with(uint8_t              *data,
                              size_t                size,
                              size_t                max_size,
                              unsigned int          seed,
                              fuzz_input_mutator_fn mutate_input);
