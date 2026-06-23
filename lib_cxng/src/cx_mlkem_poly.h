#ifndef MLKEM_POLY_H
#define MLKEM_POLY_H

#include "lcx_mlkem.h"

/**
 * @brief   Polynomial with MLKEM_N coefficients.
 */
typedef struct {
    int16_t coeffs[MLKEM_N];
} poly;

/**
 * @brief   Montgomery reduction of a 32-bit integer.
 *
 * @param[in] a  Input value.
 *
 * @return       Reduced value in [-q/2, q/2].
 */
int16_t MLKEM_POLY_montgomery_reduce(int32_t a);

/**
 * @brief   Multiplication in the NTT domain followed by Montgomery reduction.
 *
 * @param[in] a  First operand.
 * @param[in] b  Second operand.
 *
 * @return       Product reduced modulo q.
 */
int16_t MLKEM_POLY_fqmul(int16_t a, int16_t b);

/**
 * @brief   Barrett reduction of a coefficient modulo q.
 *
 * @param[in] a  Input coefficient.
 *
 * @return       Reduced coefficient.
 */
int16_t MLKEM_POLY_barrett_reduce(int16_t a);

/**
 * @brief   Maps a signed representative to an unsigned one in [0, q-1].
 *
 * @param[in] c  Signed coefficient.
 *
 * @return       Unsigned representative modulo q.
 */
int16_t MLKEM_POLY_signed_to_unsigned_q(int16_t c);

/**
 * @brief   Applies Barrett reduction to all coefficients and map to [0, q-1].
 *
 * @param[in,out] r  Polynomial to reduce in place.
 */
void MLKEM_POLY_reduce(poly *r);

/**
 * @brief   Converts a polynomial to Montgomery domain.
 *
 * @param[in,out] r  Polynomial to convert in place.
 */
void MLKEM_POLY_tomont(poly *r);

/**
 * @brief   Adds polynomial b to polynomial r in place.
 *
 * @param[in,out] r  Accumulator polynomial.
 * @param[in]     b  Polynomial to add.
 */
void MLKEM_POLY_add(poly *r, const poly *b);

/**
 * @brief   Subtracts polynomial b from polynomial r in place.
 *
 * @param[in,out] r  Accumulator polynomial.
 * @param[in]     b  Polynomial to subtract.
 */
void MLKEM_POLY_sub(poly *r, const poly *b);

/**
 * @brief   Computes the NTT of a polynomial in place.
 *
 * @param[in,out] p  Polynomial to transform.
 */
void MLKEM_POLY_ntt(poly *p);

/**
 * @brief   Computes the inverse NTT and multiply by Montgomery factor.
 *
 * @param[in,out] p  Polynomial to transform in place.
 */
void MLKEM_POLY_invntt_tomont(poly *p);

/**
 * @brief   Pointwise multiplication of two polynomials in NTT domain
 *          with accumulation.
 *
 * @param[in,out] r      Accumulator polynomial (set or accumulated depending on @p first).
 * @param[in]     a      First input polynomial.
 * @param[in]     b      Second input polynomial.
 * @param[in]     first  If non-zero, overwrite r; otherwise accumulate into r.
 */
void MLKEM_POLY_basemul_acc_montgomery(poly *r, const poly *a, const poly *b, int32_t first);

/**
 * @brief   Serializes a polynomial to bytes (12 bits per coefficient).
 *
 * @param[out] r  Output byte array of MLKEM_POLYBYTES bytes.
 * @param[in]  a  Polynomial to serialize.
 */
void MLKEM_POLY_tobytes(uint8_t *r, const poly *a);

/**
 * @brief   Deserializes a polynomial from bytes (12 bits per coefficient).
 *
 * @param[out] r  Output polynomial.
 * @param[in]  a  Input byte array of MLKEM_POLYBYTES bytes.
 */
void MLKEM_POLY_frombytes(poly *r, const uint8_t *a);

/**
 * @brief   Scalar compression with d=1.
 *
 * @param[in] u  Coefficient to compress.
 *
 * @return       Compressed value (1 bit).
 */
uint8_t MLKEM_POLY_scalar_compress_d1(int16_t u);

/**
 * @brief   Scalar compression with d=4.
 *
 * @param[in] u  Coefficient to compress.
 *
 * @return       Compressed value (4 bits).
 */
uint8_t MLKEM_POLY_scalar_compress_d4(int16_t u);

/**
 * @brief   Scalar decompression with d=4.
 *
 * @param[in] u  Compressed value (4 bits).
 *
 * @return       Decompressed coefficient.
 */
int16_t MLKEM_POLY_scalar_decompress_d4(uint8_t u);

/**
 * @brief   Scalar compression with d=5.
 *
 * @param[in] u  Coefficient to compress.
 *
 * @return       Compressed value (5 bits).
 */
uint8_t MLKEM_POLY_scalar_compress_d5(int16_t u);

/**
 * @brief   Scalar decompression with d=5.
 *
 * @param[in] u  Compressed value (5 bits).
 *
 * @return       Decompressed coefficient.
 */
int16_t MLKEM_POLY_scalar_decompress_d5(uint8_t u);

/**
 * @brief   Scalar compression with d=10.
 *
 * @param[in] u  Coefficient to compress.
 *
 * @return       Compressed value (10 bits).
 */
uint16_t MLKEM_POLY_scalar_compress_d10(int16_t u);

/**
 * @brief   Scalar decompression with d=10.
 *
 * @param[in] u  Compressed value (10 bits).
 *
 * @return       Decompressed coefficient.
 */
int16_t MLKEM_POLY_scalar_decompress_d10(uint16_t u);

/**
 * @brief   Scalar compression with d=11.
 *
 * @param[in] u  Coefficient to compress.
 *
 * @return       Compressed value (11 bits).
 */
uint16_t MLKEM_POLY_scalar_compress_d11(int16_t u);

/**
 * @brief   Scalar decompression with d=11.
 *
 * @param[in] u  Compressed value (11 bits).
 *
 * @return       Decompressed coefficient.
 */
int16_t MLKEM_POLY_scalar_decompress_d11(uint16_t u);

/**
 * @brief   Compresses a polynomial with dv=4 and serialize to bytes.
 *
 * @param[out] r  Output byte array.
 * @param[in]  a  Polynomial to compress.
 */
void MLKEM_POLY_compress_d4(uint8_t *r, const poly *a);

/**
 * @brief   Decompresses a polynomial from bytes with dv=4.
 *
 * @param[out] r  Output polynomial.
 * @param[in]  a  Input compressed byte array.
 */
void MLKEM_POLY_decompress_d4(poly *r, const uint8_t *a);

/**
 * @brief   Compresses a polynomial with dv=5 and serialize to bytes.
 *
 * @param[out] r  Output byte array.
 * @param[in]  a  Polynomial to compress.
 */
void MLKEM_POLY_compress_d5(uint8_t *r, const poly *a);

/**
 * @brief   Decompresses a polynomial from bytes with dv=5.
 *
 * @param[out] r  Output polynomial.
 * @param[in]  a  Input compressed byte array.
 */
void MLKEM_POLY_decompress_d5(poly *r, const uint8_t *a);

/**
 * @brief   Compresses a polynomial with du=10 and serialize to bytes.
 *
 * @param[out] r  Output byte array.
 * @param[in]  a  Polynomial to compress.
 */
void MLKEM_POLY_compress_d10(uint8_t *r, const poly *a);

/**
 * @brief   Decompresses a polynomial from bytes with du=10.
 *
 * @param[out] r  Output polynomial.
 * @param[in]  a  Input compressed byte array.
 */
void MLKEM_POLY_decompress_d10(poly *r, const uint8_t *a);

/**
 * @brief   Compresses a polynomial with du=11 and serialize to bytes.
 *
 * @param[out] r  Output byte array.
 * @param[in]  a  Polynomial to compress.
 */
void MLKEM_POLY_compress_d11(uint8_t *r, const poly *a);

/**
 * @brief   Decompresses a polynomial from bytes with du=11.
 *
 * @param[out] r  Output polynomial.
 * @param[in]  a  Input compressed byte array.
 */
void MLKEM_POLY_decompress_d11(poly *r, const uint8_t *a);

/**
 * @brief   Decodes a 32-byte message into a polynomial.
 *
 * @param[out] r    Output polynomial.
 * @param[in]  msg  Input message of MLKEM_SYMBYTES bytes.
 */
void MLKEM_POLY_frommsg(poly *r, const uint8_t *msg);

/**
 * @brief   Encodes a polynomial into a 32-byte message.
 *
 * @param[out] msg  Output message of MLKEM_SYMBYTES bytes.
 * @param[in]  a    Polynomial to encode.
 */
void MLKEM_POLY_tomsg(uint8_t *msg, const poly *a);

#endif  // MLKEM_POLY_H
