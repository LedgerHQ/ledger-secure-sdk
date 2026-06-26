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
#ifndef CX_MLDSA_POLYVEC_H
#define CX_MLDSA_POLYVEC_H

#include "cx_mldsa_poly.h"
#include "lcx_mldsa.h"

/**
 * @brief   Polynomial vector of up to MLDSA_MAX_L polynomials.
 */
typedef struct {
    mldsa_poly vec[MLDSA_MAX_L];
} mldsa_polyvecl;

/**
 * @brief   Polynomial vector of up to MLDSA_MAX_K polynomials.
 */
typedef struct {
    mldsa_poly vec[MLDSA_MAX_K];
} mldsa_polyveck;

/**
 * @brief   Apply NTT to all polynomials in an L-vector.
 */
void MLDSA_POLYVEC_ntt_l(mldsa_polyvecl *v, uint8_t l);

/**
 * @brief   Apply NTT to all polynomials in a K-vector.
 */
void MLDSA_POLYVEC_ntt_k(mldsa_polyveck *v, uint8_t k);

/**
 * @brief   Apply inverse NTT to all polynomials in a K-vector.
 */
void MLDSA_POLYVEC_invntt_tomont_k(mldsa_polyveck *v, uint8_t k);

/**
 * @brief   Inner product of L-vectors in NTT domain with accumulation.
 *
 * @param[out] w  Output polynomial.
 * @param[in]  u  First input L-vector.
 * @param[in]  v  Second input L-vector.
 * @param[in]  l  Number of polynomials.
 */
void MLDSA_POLYVEC_pointwise_acc_montgomery(mldsa_poly           *w,
                                            const mldsa_polyvecl *u,
                                            const mldsa_polyvecl *v,
                                            uint8_t               l);

/**
 * @brief   Add K-vector b to K-vector a.
 */
void MLDSA_POLYVEC_add_k(mldsa_polyveck *a, const mldsa_polyveck *b, uint8_t k);

/**
 * @brief   Subtract K-vector b from K-vector a.
 */
void MLDSA_POLYVEC_sub_k(mldsa_polyveck *a, const mldsa_polyveck *b, uint8_t k);

/**
 * @brief   Apply reduce to all polynomials in a K-vector.
 */
void MLDSA_POLYVEC_reduce_k(mldsa_polyveck *v, uint8_t k);

/**
 * @brief   Apply caddq to all polynomials in a K-vector.
 */
void MLDSA_POLYVEC_caddq_k(mldsa_polyveck *v, uint8_t k);

/**
 * @brief   Check infinity norm of an L-vector.
 *
 * @return  0 if within bound, 1 otherwise.
 */
int MLDSA_POLYVEC_chknorm_l(const mldsa_polyvecl *v, int32_t B, uint8_t l);

/**
 * @brief   Check infinity norm of a K-vector.
 *
 * @return  0 if within bound, 1 otherwise.
 */
int MLDSA_POLYVEC_chknorm_k(const mldsa_polyveck *v, int32_t B, uint8_t k);

/**
 * @brief   Shift left all polynomials in K-vector by D bits.
 */
void MLDSA_POLYVEC_shiftl_k(mldsa_polyveck *v, uint8_t k);

/**
 * @brief   Power2round all polynomials in K-vector.
 *
 * @param[out] v1  High bits output K-vector.
 * @param[out] v0  Low bits output K-vector.
 * @param[in]  v   Input K-vector.
 * @param[in]  k   Number of polynomials.
 */
void MLDSA_POLYVEC_power2round_k(mldsa_polyveck       *v1,
                                 mldsa_polyveck       *v0,
                                 const mldsa_polyveck *v,
                                 uint8_t               k);

/**
 * @brief   Decompose all polynomials in K-vector.
 */
void MLDSA_POLYVEC_decompose_k(mldsa_polyveck       *v1,
                               mldsa_polyveck       *v0,
                               const mldsa_polyveck *v,
                               int32_t               gamma2,
                               uint8_t               k);

/**
 * @brief   Make hint for K-vectors.
 *
 * @param[out] h       Hint K-vector.
 * @param[in]  v0      Low bits K-vector.
 * @param[in]  v1      High bits K-vector.
 * @param[in]  gamma2  Low-order rounding range.
 * @param[in]  k       Number of polynomials.
 *
 * @return             Number of ones in hint.
 */
uint32_t MLDSA_POLYVEC_make_hint_k(mldsa_polyveck       *h,
                                   const mldsa_polyveck *v0,
                                   const mldsa_polyveck *v1,
                                   int32_t               gamma2,
                                   uint8_t               k);

/**
 * @brief   Use hint to correct high bits of K-vector.
 */
void MLDSA_POLYVEC_use_hint_k(mldsa_polyveck       *w,
                              const mldsa_polyveck *u,
                              const mldsa_polyveck *h,
                              int32_t               gamma2,
                              uint8_t               k);

/**
 * @brief   Pack w1 vector.
 *
 * @param[out] r       Output byte array.
 * @param[in]  w1      Input K-vector.
 * @param[in]  gamma2  Low-order rounding range.
 * @param[in]  k       Number of polynomials.
 */
void MLDSA_POLYVEC_pack_w1(uint8_t *r, const mldsa_polyveck *w1, int32_t gamma2, uint8_t k);

#endif /* CX_MLDSA_POLYVEC_H */
