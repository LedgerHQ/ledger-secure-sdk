
/*******************************************************************************
 *   Ledger Nano S - Secure firmware
 *   (c) 2022 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

/**
 * @file    lcx_math.h
 * @brief   Basic arithmetic.
 */

#ifndef LCX_MATH_H
#define LCX_MATH_H

#ifdef HAVE_MATH

#include "lcx_wrappers.h"
#include "ox_bn.h"

/**
 * @brief Compares two integers represented as byte arrays.
 *
 * @param[in]  a      Pointer to the first integer.
 *
 * @param[in]  b      Pointer to the second integer.
 *
 * @param[in]  length Number of bytes taken into account for the comparison.
 *
 * @param[out] diff   Result of the comparison:
 *                    - 0 if a and b are identical
 *                    - < 0 if a is less than b
 *                    - > 0 if a is greater than b
 *
 * @return            Error code:
 *                    - CX_OK on success
 *                    - CX_NOT_UNLOCKED
 *                    - CX_INVALID_PARAMETER_SIZE
 *                    - CX_NOT_LOCKED
 *                    - CX_MEMORY_FULL
 *                    - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_cmp_no_throw(const uint8_t *a,
                                                 const uint8_t *b,
                                                 size_t         length,
                                                 int           *diff);

/**
 * @deprecated
 * See #cx_math_cmp_no_throw
 */
DEPRECATED static inline int32_t cx_math_cmp(const uint8_t *a, const uint8_t *b, size_t length)
{
    int diff;
    CX_THROW(cx_math_cmp_no_throw(a, b, length, &diff));
    return diff;
}

/**
 * @brief Adds two integers represented as byte arrays.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the first integer.
 *
 * @param[in]  b   Pointer to the second integer.
 *
 * @param[in]  len Number of bytes taken into account for the addition.
 *
 * @return         Error code:
 *                 - CX_OK on success
 *                 - CX_NOT_UNLOCKED
 *                 - CX_INVALID_PARAMETER_SIZE
 *                 - CX_NOT_LOCKED
 *                 - CX_MEMORY_FULL
 *                 - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_add_no_throw(uint8_t       *r,
                                                 const uint8_t *a,
                                                 const uint8_t *b,
                                                 size_t         len);

/**
 * @brief   Adds two integers represented as byte arrays.
 *
 * @details This function throws an exception if the computation
 *          doesn't succeed.
 *
 * @warning It is recommended to use #cx_math_add_no_throw rather
 *          than this function.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the first integer.
 *
 * @param[in]  b   Pointer to the second integer.
 *
 * @param[in]  len Number of bytes taken into account for the addition.
 *
 * @return         1 if there is a carry, 0 otherwise.
 *
 * @throws         CX_NOT_UNLOCKED
 * @throws         CX_INVALID_PARAMETER_SIZE
 * @throws         CX_NOT_LOCKED
 * @throws         CX_MEMORY_FULL
 * @throws         CX_INVALID_PARAMETER
 */
static inline uint32_t cx_math_add(uint8_t *r, const uint8_t *a, const uint8_t *b, size_t len)
{
    cx_err_t error = cx_math_add_no_throw(r, a, b, len);
    if (error && error != CX_CARRY) {
        THROW(error);
    }
    return (error == CX_CARRY);
}

/**
 * @brief Subtracts two integers represented as byte arrays.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the first integer.
 *
 * @param[in]  b   Pointer to the second integer.
 *
 * @param[in]  len Number of bytes taken into account for the subtraction.
 *
 * @return         Error code:
 *                 - CX_OK on success
 *                 - CX_NOT_UNLOCKED
 *                 - CX_INVALID_PARAMETER_SIZE
 *                 - CX_NOT_LOCKED
 *                 - CX_MEMORY_FULL
 *                 - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_sub_no_throw(uint8_t       *r,
                                                 const uint8_t *a,
                                                 const uint8_t *b,
                                                 size_t         len);

/**
 * @brief   Subtracts two integers represented as byte arrays.
 *
 * @details This function throws an exception if the computation
 *          doesn't succeed.
 *
 * @warning It is recommended to use #cx_math_sub_no_throw rather
 *          than this function.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the first integer.
 *
 * @param[in]  b   Pointer to the second integer.
 *
 * @param[in]  len Number of bytes taken into account for the subtraction.
 *
 * @return         1 if there is a carry, 0 otherwise.
 *
 * @throws         CX_NOT_UNLOCKED
 * @throws         CX_INVALID_PARAMETER_SIZE
 * @throws         CX_NOT_LOCKED
 * @throws         CX_MEMORY_FULL
 * @throws         CX_INVALID_PARAMETER
 */
static inline uint32_t cx_math_sub(uint8_t *r, const uint8_t *a, const uint8_t *b, size_t len)
{
    cx_err_t error = cx_math_sub_no_throw(r, a, b, len);
    if (error && error != CX_CARRY) {
        THROW(error);
    }
    return (error == CX_CARRY);
}

/**
 * @brief Multiplies two integers represented as byte arrays.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the first integer.
 *
 * @param[in]  b   Pointer to the second integer.
 *
 * @param[in]  len Number of bytes taken into account for the multiplication.
 *
 * @return         Error code:
 *                 - CX_OK on success
 *                 - CX_NOT_UNLOCKED
 *                 - CX_INVALID_PARAMETER_SIZE
 *                 - CX_NOT_LOCKED
 *                 - CX_MEMORY_FULL
 *                 - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_mult_no_throw(uint8_t       *r,
                                                  const uint8_t *a,
                                                  const uint8_t *b,
                                                  size_t         len);

/**
 * @deprecated
 * See #cx_math_mult_no_throw
 */
DEPRECATED static inline void cx_math_mult(uint8_t       *r,
                                           const uint8_t *a,
                                           const uint8_t *b,
                                           size_t         len)
{
    CX_THROW(cx_math_mult_no_throw(r, a, b, len));
}

/**
 * @brief Performs a modular addition
 *        of two integers represented as byte arrays.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the first integer.
 *                 This must be strictly smaller than the modulus.
 *
 * @param[in]  b   Pointer to the second integer.
 *                 This must be strictly smaller than the modulus.
 *
 * @param[in]  m   Modulus
 *
 * @param[in]  len Number of bytes taken into account for the operation.
 *
 * @return         Error code:
 *                 - CX_OK on success
 *                 - CX_NOT_UNLOCKED
 *                 - CX_INVALID_PARAMETER_SIZE
 *                 - CX_NOT_LOCKED
 *                 - CX_MEMORY_FULL
 *                 - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t
cx_math_addm_no_throw(uint8_t *r, const uint8_t *a, const uint8_t *b, const uint8_t *m, size_t len);

/**
 * @deprecated
 * See #cx_math_addm_no_throw
 */
DEPRECATED static inline void cx_math_addm(uint8_t       *r,
                                           const uint8_t *a,
                                           const uint8_t *b,
                                           const uint8_t *m,
                                           size_t         len)
{
    CX_THROW(cx_math_addm_no_throw(r, a, b, m, len));
}

/**
 * @brief Performs a modular subtraction of
 *        two integers represented as byte arrays.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the first integer.
 *                 This must be strictly smaller than the modulus.
 *
 * @param[in]  b   Pointer to the second integer.
 *                 This must be strictly smaller than the modulus.
 *
 * @param[in]  m   Modulus
 *
 * @param[in]  len Number of bytes taken into account for the operation.
 *
 * @return         Error code:
 *                 - CX_OK on success
 *                 - CX_NOT_UNLOCKED
 *                 - CX_INVALID_PARAMETER_SIZE
 *                 - CX_NOT_LOCKED
 *                 - CX_MEMORY_FULL
 *                 - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t
cx_math_subm_no_throw(uint8_t *r, const uint8_t *a, const uint8_t *b, const uint8_t *m, size_t len);

/**
 * @deprecated
 * See #cx_math_subm_no_throw
 */
DEPRECATED static inline void cx_math_subm(uint8_t       *r,
                                           const uint8_t *a,
                                           const uint8_t *b,
                                           const uint8_t *m,
                                           size_t         len)
{
    CX_THROW(cx_math_subm_no_throw(r, a, b, m, len));
}

/**
 * @brief Performs a modular multiplication of
 *        two integers represented as byte arrays.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the first integer.
 *
 * @param[in]  b   Pointer to the second integer.
 *                 This must be strictly smaller than the modulus.
 *
 * @param[in]  m   Modulus
 *
 * @param[in]  len Number of bytes taken into account for the operation.
 *
 * @return         Error code:
 *                 - CX_OK on success
 *                 - CX_NOT_UNLOCKED
 *                 - CX_INVALID_PARAMETER_SIZE
 *                 - CX_NOT_LOCKED
 *                 - CX_MEMORY_FULL
 *                 - CX_INVALID_PARAMETER
 *                 - CX_INVALID_PARAMETER_VALUE
 */
WARN_UNUSED_RESULT cx_err_t cx_math_multm_no_throw(uint8_t       *r,
                                                   const uint8_t *a,
                                                   const uint8_t *b,
                                                   const uint8_t *m,
                                                   size_t         len);

/**
 * @deprecated
 * See #cx_math_multm_no_throw
 */
DEPRECATED static inline void cx_math_multm(uint8_t       *r,
                                            const uint8_t *a,
                                            const uint8_t *b,
                                            const uint8_t *m,
                                            size_t         len)
{
    CX_THROW(cx_math_multm_no_throw(r, a, b, m, len));
}

/**
 * @brief   Performs a modular reduction.
 *
 * @details Computes the remainder of the division of v by m. Store the result in v.
 *
 * @param[in,out] v     Pointer to the dividend and buffer for the result.
 *
 * @param[in]     len_v Number of bytes of the dividend.
 *
 * @param[in]     m     Modulus.
 *
 * @param[in]     len_m Number of bytes of the modulus.
 *
 * @return              Error code:
 *                      - CX_OK on success
 *                      - CX_NOT_UNLOCKED
 *                      - CX_INVALID_PARAMETER_SIZE
 *                      - CX_NOT_LOCKED
 *                      - CX_MEMORY_FULL
 *                      - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_modm_no_throw(uint8_t       *v,
                                                  size_t         len_v,
                                                  const uint8_t *m,
                                                  size_t         len_m);

/**
 * @deprecated
 * See #cx_math_modm_no_throw
 */
DEPRECATED static inline void cx_math_modm(uint8_t *v, size_t len_v, const uint8_t *m, size_t len_m)
{
    CX_THROW(cx_math_modm_no_throw(v, len_v, m, len_m));
}

/**
 * @brief   Performs a modular exponentiation.
 *
 * @details Computes the result of **a^e mod m**.
 *
 * @param[out] r     Buffer for the result.
 *
 * @param[in]  a     Pointer to an integer.
 *
 * @param[in]  e     Pointer to the exponent.
 *
 * @param[in]  len_e Number of bytes of the exponent.
 *
 * @param[in]  m     Modulus
 *
 * @param[in]  len   Number of bytes of the result.
 *
 * @return           Error code:
 *                   - CX_OK on success
 *                   - CX_NOT_UNLOCKED
 *                   - CX_INVALID_PARAMETER_SIZE
 *                   - CX_NOT_LOCKED
 *                   - CX_MEMORY_FULL
 *                   - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_powm_no_throw(uint8_t       *r,
                                                  const uint8_t *a,
                                                  const uint8_t *e,
                                                  size_t         len_e,
                                                  const uint8_t *m,
                                                  size_t         len);

/**
 * @deprecated
 * See #cx_math_powm_no_throw
 */
DEPRECATED static inline void cx_math_powm(uint8_t       *r,
                                           const uint8_t *a,
                                           const uint8_t *e,
                                           size_t         len_e,
                                           const uint8_t *m,
                                           size_t         len)
{
    CX_THROW(cx_math_powm_no_throw(r, a, e, len_e, m, len));
}

/**
 * @brief   Computes the modular inverse with a prime modulus.
 *
 * @details It computes the result of **a^(-1) mod m**, for a prime *m*.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the integer.
 *
 * @param[in]  m   Modulus. Must be a prime number.
 *
 * @param[in]  len Number of bytes of the result.
 *
 * @return         Error code:
 *                 - CX_OK on success
 *                 - CX_NOT_UNLOCKED
 *                 - CX_INVALID_PARAMETER_SIZE
 *                 - CX_NOT_LOCKED
 *                 - CX_MEMORY_FULL
 *                 - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_invprimem_no_throw(uint8_t       *r,
                                                       const uint8_t *a,
                                                       const uint8_t *m,
                                                       size_t         len);

/**
 * @deprecated
 * See #cx_math_invprimem_no_throw
 */
DEPRECATED static inline void cx_math_invprimem(uint8_t       *r,
                                                const uint8_t *a,
                                                const uint8_t *m,
                                                size_t         len)
{
    CX_THROW(cx_math_invprimem_no_throw(r, a, m, len));
}

/**
 * @brief   Computes the modular inverse.
 *
 * @details It computes the result of **a^(-1) mod m**. *a* must be invertible modulo *m*,
 *          i.e. the greatest common divisor of *a* and *m* is 1.
 *
 * @param[out] r   Buffer for the result.
 *
 * @param[in]  a   Pointer to the integer.
 *
 * @param[in]  m   Modulus.
 *
 * @param[in]  len Number of bytes of the result.
 *
 * @return         Error code:
 *                 - CX_OK on success
 *                 - CX_NOT_UNLOCKED
 *                 - CX_INVALID_PARAMETER_SIZE
 *                 - CX_NOT_LOCKED
 *                 - CX_MEMORY_FULL
 *                 - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_invintm_no_throw(uint8_t       *r,
                                                     uint32_t       a,
                                                     const uint8_t *m,
                                                     size_t         len);

/**
 * @deprecated
 * See #cx_math_invintm_no_throw
 */
DEPRECATED static inline void cx_math_invintm(uint8_t *r, uint32_t a, const uint8_t *m, size_t len)
{
    CX_THROW(cx_math_invintm_no_throw(r, a, m, len));
}

/**
 * @brief Checks whether a number is probably prime.
 *
 * @param[in]  r     Pointer to an integer.
 *
 * @param[in]  len   Number of bytes of the integer.
 *
 * @param[out] prime Bool indicating whether r is prime or not:
 *                   - 0 : not prime
 *                   - 1 : prime
 *
 * @return           Error code:
 *                   - CX_OK on success
 *                   - CX_NOT_UNLOCKED
 *                   - CX_INVALID_PARAMETER_SIZE
 *                   - CX_NOT_LOCKED
 *                   - CX_MEMORY_FULL
 *                   - CX_INVALID_PARAMETER
 */
WARN_UNUSED_RESULT cx_err_t cx_math_is_prime_no_throw(const uint8_t *r, size_t len, bool *prime);

/**
 * @deprecated
 * See #cx_math_is_prime_no_throw
 */
DEPRECATED static inline bool cx_math_is_prime(const uint8_t *r, size_t len)
{
    bool prime;
    CX_THROW(cx_math_is_prime_no_throw(r, len, &prime));
    return prime;
}

/**
 * @brief Computes the next prime after a given number.
 *
 * @param[in, out] r   Pointer to the integer and buffer for the result.
 *
 * @param[in]      len Number of bytes of the integer.
 *
 * @return             Error code:
 *                     - CX_OK on success
 *                     - CX_NOT_UNLOCKED
 *                     - CX_INVALID_PARAMETER_SIZE
 *                     - CX_MEMORY_FULL
 *                     - CX_NOT_LOCKED
 *                     - CX_INVALID_PARAMETER
 *                     - CX_INTERNAL_ERROR
 *                     - CX_OVERFLOW
 */
WARN_UNUSED_RESULT cx_err_t cx_math_next_prime_no_throw(uint8_t *r, uint32_t len);

/**
 * @deprecated
 * See #cx_math_next_prime_no_throw
 */
DEPRECATED static inline void cx_math_next_prime(uint8_t *r, uint32_t len)
{
    CX_THROW(cx_math_next_prime_no_throw(r, len));
}

/**
 * @brief Checks whether the byte array of an integer is all zero.
 *
 * @param[in] a   Pointer to an integer.
 *
 * @param[in] len Number of bytes of the integer.
 *
 * @return        1 if a is all zero, 0 otherwise.
 */
static inline bool cx_math_is_zero(const uint8_t *a, size_t len)
{
    uint8_t acc = 0;  // accumulate all the bytes in order to run in constant time
    for (size_t i = 0; i < len; i++) {
        acc |= a[i];
    }
    return acc == 0;
}

#endif  // HAVE_MATH

#endif  // LCX_MATH_H
