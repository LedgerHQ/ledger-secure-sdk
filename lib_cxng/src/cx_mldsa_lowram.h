/*****************************************************************************
 *   (c) 2026 Ledger SAS.
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
 *****************************************************************************/
/**
 * @file    cx_mldsa_lowram.h
 * @brief   ML-DSA low-RAM helper functions.
 *
 * Provides compressed polynomial buffers, schoolbook multiplication from
 * packed format, fused A expansion with pointwise accumulation, and
 * streaming gamma1 sampling.
 */

#ifndef CX_MLDSA_LOWRAM_H
#define CX_MLDSA_LOWRAM_H

#ifdef HAVE_MLDSA_OPTIMIZATION

#include <stdint.h>
#include "lcx_mldsa.h"
#include "cx_mldsa_poly.h"

/** Compressed polynomial buffer: 3 bytes per coefficient = 768 bytes. */
#define MLDSA_WCOMP_BYTES 768U

/** Compressed challenge: TAU positions (max 60) + 8 sign bytes = 68 bytes. */
#define MLDSA_CCOMP_BYTES 68U

/** Maximum omega across all parameter sets. */
#define MLDSA_MAX_OMEGA 80U

/**
 * @brief   Pack a polynomial into compressed 3-byte-per-coefficient format.
 *
 * The polynomial must have coefficients in [0, q) (i.e., after caddq).
 *
 * @param[out] buf  Output buffer (MLDSA_WCOMP_BYTES bytes).
 * @param[in]  w    Input polynomial.
 */
void MLDSA_LOWRAM_polyw_pack(uint8_t buf[MLDSA_WCOMP_BYTES], const mldsa_poly *w);

/**
 * @brief   Unpack a compressed polynomial into a full polynomial.
 *
 * @param[out] w    Output polynomial.
 * @param[in]  buf  Input compressed buffer (MLDSA_WCOMP_BYTES bytes).
 */
void MLDSA_LOWRAM_polyw_unpack(mldsa_poly *w, const uint8_t buf[MLDSA_WCOMP_BYTES]);

/**
 * @brief   Add a value to a single coefficient in a compressed polynomial buffer.
 *
 * @param[in,out] buf  Compressed buffer.
 * @param[in]     a    Value to add (will be reduced mod q).
 * @param[in]     idx  Coefficient index [0, 255].
 */
void MLDSA_LOWRAM_polyw_add_idx(uint8_t buf[MLDSA_WCOMP_BYTES], int32_t a, uint32_t idx);

/**
 * @brief   Subtract polynomial from compressed buffer: c = buf - a.
 *
 * @param[out] c    Output polynomial.
 * @param[in]  buf  Compressed buffer.
 * @param[in]  a    Polynomial to subtract.
 */
void MLDSA_LOWRAM_polyw_sub(mldsa_poly       *c,
                            const uint8_t     buf[MLDSA_WCOMP_BYTES],
                            const mldsa_poly *a);

/**
 * @brief   Compress a challenge polynomial into 68 bytes.
 *
 * Format: positions[0..TAU-1] || signs[8 bytes as uint64 LE].
 *
 * @param[out] ccomp  Output buffer (MLDSA_CCOMP_BYTES bytes).
 * @param[in]  cp     Challenge polynomial (TAU nonzero coefficients in {-1, +1}).
 * @param[in]  tau    Number of nonzero coefficients.
 */
void MLDSA_LOWRAM_challenge_compress(uint8_t           ccomp[MLDSA_CCOMP_BYTES],
                                     const mldsa_poly *cp,
                                     uint8_t           tau);

/**
 * @brief   Decompress a challenge from 68 bytes back into a full polynomial.
 *
 * @param[out] cp     Output polynomial.
 * @param[in]  ccomp  Compressed challenge (MLDSA_CCOMP_BYTES bytes).
 * @param[in]  tau    Number of nonzero coefficients.
 */
void MLDSA_LOWRAM_challenge_decompress(mldsa_poly   *cp,
                                       const uint8_t ccomp[MLDSA_CCOMP_BYTES],
                                       uint8_t       tau);

/**
 * @brief   Schoolbook multiply: c = compressed_challenge * packed_t0.
 *
 * Reads t0 coefficients directly from their 13-bit packed representation
 * (as stored in the secret key) without fully unpacking.
 *
 * @param[out] c      Output polynomial.
 * @param[in]  ccomp  Compressed challenge (MLDSA_CCOMP_BYTES bytes).
 * @param[in]  t0     Packed t0 polynomial (MLDSA_POLYT0_PACKEDBYTES bytes).
 * @param[in]  tau    Number of nonzero coefficients in challenge.
 */
void MLDSA_LOWRAM_schoolbook_t0(mldsa_poly    *c,
                                const uint8_t  ccomp[MLDSA_CCOMP_BYTES],
                                const uint8_t *t0,
                                uint8_t        tau);

/**
 * @brief   Schoolbook multiply: c = compressed_challenge * packed_t1 * 2^D.
 *
 * Reads t1 coefficients directly from their 10-bit packed representation
 * (as stored in the public key) without fully unpacking.
 *
 * @param[out] c      Output polynomial.
 * @param[in]  ccomp  Compressed challenge (MLDSA_CCOMP_BYTES bytes).
 * @param[in]  t1     Packed t1 polynomial (MLDSA_POLYT1_PACKEDBYTES bytes).
 * @param[in]  tau    Number of nonzero coefficients in challenge.
 */
void MLDSA_LOWRAM_schoolbook_t1(mldsa_poly    *c,
                                const uint8_t  ccomp[MLDSA_CCOMP_BYTES],
                                const uint8_t *t1,
                                uint8_t        tau);

/**
 * @brief   Fused A-expansion and pointwise accumulation into compressed w buffer.
 *
 * Generates A[row][col] coefficients from SHAKE128(rho || nonce) one at a time,
 * immediately multiplies with the corresponding coefficient of b (in NTT domain),
 * and accumulates into wcomp using Montgomery reduction.
 *
 * @param[in,out] wcomp  Compressed w buffer (MLDSA_WCOMP_BYTES bytes).
 * @param[in]     b      Polynomial in NTT domain to multiply with.
 * @param[in]     rho    Seed (MLDSA_SEEDBYTES bytes).
 * @param[in]     nonce  Two-byte nonce encoding (row, col).
 */
void MLDSA_LOWRAM_expand_aij_accum(uint8_t           wcomp[MLDSA_WCOMP_BYTES],
                                   const mldsa_poly *b,
                                   const uint8_t     rho[MLDSA_SEEDBYTES],
                                   uint16_t          nonce);

/**
 * @brief   Compute high bits of a polynomial (inline decompose).
 *
 * @param[out] a1      Output polynomial (high bits).
 * @param[in]  a       Input polynomial (standard representative in [0, q)).
 * @param[in]  gamma2  Low-order rounding range.
 */
void MLDSA_LOWRAM_poly_highbits(mldsa_poly *a1, const mldsa_poly *a, int32_t gamma2);

/**
 * @brief   Compute low bits of a polynomial (inline decompose).
 *
 * @param[out] a0      Output polynomial (low bits).
 * @param[in]  a       Input polynomial (standard representative in [0, q)).
 * @param[in]  gamma2  Low-order rounding range.
 */
void MLDSA_LOWRAM_poly_lowbits(mldsa_poly *a0, const mldsa_poly *a, int32_t gamma2);

/**
 * @brief   Compute the reference r0 = LowBits(w) - c*s2 from the compressed
 *          (w - c*s2) buffer and the packed HighBits(w) row.
 *
 * The reference decomposes the original w once into (HighBits(w), LowBits(w))
 * and forms r0 = LowBits(w) - c*s2, then checks ||r0||_inf < gamma2 - beta.
 * This helper reconstructs the reference r0 from the stored (w - c*s2) and w1 = HighBits(w):
 * r0 = reduce32((w - c*s2) - w1 * 2 * gamma2) = LowBits(w) - c*s2.
 *
 * @param[out] r0         Output polynomial r0 = LowBits(w) - c*s2.
 * @param[in]  wcomp      Compressed (w - c*s2) buffer.
 * @param[in]  w1_packed  Packed HighBits(w) row (w1).
 * @param[in]  gamma2     Low-order rounding range.
 */
void MLDSA_LOWRAM_poly_r0(mldsa_poly    *r0,
                          const uint8_t  wcomp[MLDSA_WCOMP_BYTES],
                          const uint8_t *w1_packed,
                          int32_t        gamma2);

/**
 * @brief   Compute make_hint from compressed w buffer and ct0 polynomial.
 *
 * Replicates the reference computation MakeHint(LowBits(w) - c*s2 + c*t0,
 * HighBits(w)). The high-bits reference w1 = HighBits(w) is taken from the
 * packed w1 row (it must be the *original* w, not w - c*s2, since subtracting
 * c*s2 can cross a gamma2 boundary).
 *
 * @param[out] h          Output hint polynomial (coefficients 0 or 1).
 * @param[in]  ct0        c*t0 polynomial.
 * @param[in]  wcomp      Compressed (w - c*s2) buffer.
 * @param[in]  w1_packed  Packed HighBits(w) row (w1).
 * @param[in]  gamma2     Low-order rounding range.
 *
 * @return                Number of hints (ones).
 */
uint32_t MLDSA_LOWRAM_make_hint(mldsa_poly       *h,
                                const mldsa_poly *ct0,
                                const uint8_t     wcomp[MLDSA_WCOMP_BYTES],
                                const uint8_t    *w1_packed,
                                int32_t           gamma2);

/**
 * @brief   Use hint with index-list representation in verify.
 *
 * Only positions listed in h_indices are hinted (=1), all others are 0.
 *
 * @param[out] b              Output polynomial (corrected high bits).
 * @param[in]  a              Input polynomial.
 * @param[in]  h_indices      Array of hinted coefficient indices.
 * @param[in]  num_hints      Number of valid entries in h_indices.
 * @param[in]  gamma2         Low-order rounding range.
 */
void MLDSA_LOWRAM_use_hint_indices(mldsa_poly       *b,
                                   const mldsa_poly *a,
                                   const uint8_t    *h_indices,
                                   uint32_t          num_hints,
                                   int32_t           gamma2);

/**
 * @brief   Stream-sample gamma1 polynomial one coefficient at a time.
 *
 * Uses minimal buffer (5-9 bytes) instead of the full 576/640 byte buffer.
 *
 * @param[out] a       Output polynomial.
 * @param[in]  seed    Seed (MLDSA_CRHBYTES bytes).
 * @param[in]  nonce   16-bit nonce.
 * @param[in]  gamma1  Coefficient range (2^17 or 2^19).
 */
void MLDSA_LOWRAM_sample_gamma1(mldsa_poly   *a,
                                const uint8_t seed[MLDSA_CRHBYTES],
                                uint16_t      nonce,
                                int32_t       gamma1);

/**
 * @brief   Stream-sample gamma1 and add to existing polynomial.
 *
 * Computes a[i] = b[i] + gamma1_sample[i] for all i.
 *
 * @param[out] a       Output polynomial (result).
 * @param[in]  b       Polynomial to add to.
 * @param[in]  seed    Seed (MLDSA_CRHBYTES bytes).
 * @param[in]  nonce   16-bit nonce.
 * @param[in]  gamma1  Coefficient range.
 */
void MLDSA_LOWRAM_sample_gamma1_add(mldsa_poly       *a,
                                    const mldsa_poly *b,
                                    const uint8_t     seed[MLDSA_CRHBYTES],
                                    uint16_t          nonce,
                                    int32_t           gamma1);

#endif /* HAVE_MLDSA_OPTIMIZATION */
#endif /* CX_MLDSA_LOWRAM_H */
