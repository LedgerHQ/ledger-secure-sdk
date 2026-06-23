#ifndef MLKEM_POLYVEC_H
#define MLKEM_POLYVEC_H

#include "lcx_mlkem.h"
#include "cx_mlkem_poly.h"

/**
 * @brief   Vector of polynomials with up to MLKEM_MAX_K elements.
 */
typedef struct {
    poly vec[MLKEM_MAX_K];
} polyvec;

/**
 * @brief   Serializes a polynomial vector to bytes.
 *
 * @param[out] r  Output byte array.
 * @param[in]  a  Polynomial vector to serialize.
 * @param[in]  k  Number of polynomials in the vector.
 */
void MLKEM_POLYVEC_tobytes(uint8_t *r, const polyvec *a, uint8_t k);

/**
 * @brief   Deserializes a polynomial vector from bytes.
 *
 * @param[out] r  Output polynomial vector.
 * @param[in]  a  Input byte array.
 * @param[in]  k  Number of polynomials in the vector.
 */
void MLKEM_POLYVEC_frombytes(polyvec *r, const uint8_t *a, uint8_t k);

/**
 * @brief   Applies the NTT to each polynomial in a vector.
 *
 * @param[in,out] r  Polynomial vector to transform in place.
 * @param[in]     k  Number of polynomials in the vector.
 */
void MLKEM_POLYVEC_ntt(polyvec *r, uint8_t k);

/**
 * @brief   Applies the inverse NTT to each polynomial in a vector.
 *
 * @param[in,out] r  Polynomial vector to transform in place.
 * @param[in]     k  Number of polynomials in the vector.
 */
void MLKEM_POLYVEC_invntt_tomont(polyvec *r, uint8_t k);

/**
 * @brief   Reduces all coefficients of each polynomial in a vector.
 *
 * @param[in,out] r  Polynomial vector to reduce in place.
 * @param[in]     k  Number of polynomials in the vector.
 */
void MLKEM_POLYVEC_reduce(polyvec *r, uint8_t k);

/**
 * @brief   Adds polynomial vector b to r in place.
 *
 * @param[in,out] r  Accumulator polynomial vector.
 * @param[in]     b  Polynomial vector to add.
 * @param[in]     k  Number of polynomials in the vectors.
 */
void MLKEM_POLYVEC_add(polyvec *r, const polyvec *b, uint8_t k);

/**
 * @brief   Converts each polynomial in a vector to Montgomery domain.
 *
 * @param[in,out] r  Polynomial vector to convert in place.
 * @param[in]     k  Number of polynomials in the vector.
 */
void MLKEM_POLYVEC_tomont(polyvec *r, uint8_t k);

/**
 * @brief   Compresses and serializes a polynomial vector.
 *
 * @param[out] r   Output byte array.
 * @param[in]  a   Polynomial vector to compress.
 * @param[in]  k   Number of polynomials in the vector.
 * @param[in]  du  Compression parameter (10 or 11).
 */
void MLKEM_POLYVEC_compress(uint8_t *r, const polyvec *a, uint8_t k, uint8_t du);

/**
 * @brief   Decompresses and deserializes a polynomial vector.
 *
 * @param[out] r   Output polynomial vector.
 * @param[in]  a   Input compressed byte array.
 * @param[in]  k   Number of polynomials in the vector.
 * @param[in]  du  Compression parameter (10 or 11).
 */
void MLKEM_POLYVEC_decompress(polyvec *r, const uint8_t *a, uint8_t k, uint8_t du);

/**
 * @brief   Accumulated pointwise multiplication of two polynomial vectors
 *          in the NTT domain.
 *
 * @param[out] r  Result polynomial (inner product of a and b).
 * @param[in]  a  First polynomial vector.
 * @param[in]  b  Second polynomial vector.
 * @param[in]  k  Number of polynomials in the vectors.
 */
void MLKEM_POLYVEC_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b, uint8_t k);

#endif  // MLKEM_POLYVEC_H
