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

#ifndef CX_MLDSA_PACKING_H
#define CX_MLDSA_PACKING_H

#include "cx_mldsa_poly.h"
#include "lcx_mldsa.h"

/**
 * @brief   Bit-pack polynomial with coefficients a1 from power2round
 *          (10-bit values). Each coefficient uses 10 bits.
 *
 * @param[out] r  Output byte array (MLDSA_POLYT1_PACKEDBYTES bytes).
 * @param[in]  a  Input polynomial.
 */
void MLDSA_PACK_polyt1(uint8_t r[MLDSA_POLYT1_PACKEDBYTES], const mldsa_poly *a);

/**
 * @brief   Unpack polynomial t1 from bytes.
 *
 * @param[out] r  Output polynomial.
 * @param[in]  a  Input byte array.
 */
void MLDSA_PACK_unpack_polyt1(mldsa_poly *r, const uint8_t a[MLDSA_POLYT1_PACKEDBYTES]);

/**
 * @brief   Bit-pack polynomial with coefficients a0 from power2round.
 *          Coefficients in (-(2^{D-1}-1), 2^{D-1}], using 13 bits.
 *
 * @param[out] r  Output byte array (MLDSA_POLYT0_PACKEDBYTES bytes).
 * @param[in]  a  Input polynomial.
 */
void MLDSA_PACK_polyt0(uint8_t r[MLDSA_POLYT0_PACKEDBYTES], const mldsa_poly *a);

/**
 * @brief   Unpack polynomial t0 from bytes.
 *
 * @param[out] r  Output polynomial.
 * @param[in]  a  Input byte array.
 */
void MLDSA_PACK_unpack_polyt0(mldsa_poly *r, const uint8_t a[MLDSA_POLYT0_PACKEDBYTES]);

/**
 * @brief   Bit-pack polynomial with coefficients in [-eta, eta].
 *
 * @param[out] r    Output byte array.
 * @param[in]  a    Input polynomial.
 * @param[in]  eta  Range parameter (2 or 4).
 *
 * @return          Number of bytes written.
 */
uint32_t MLDSA_PACK_polyeta(uint8_t *r, const mldsa_poly *a, uint8_t eta);

/**
 * @brief   Unpack polynomial with coefficients in [-eta, eta].
 *
 * @param[out] r    Output polynomial.
 * @param[in]  a    Input byte array.
 * @param[in]  eta  Range parameter (2 or 4).
 *
 * @return          Number of bytes consumed.
 */
uint32_t MLDSA_PACK_unpack_polyeta(mldsa_poly *r, const uint8_t *a, uint8_t eta);

/**
 * @brief   Bit-pack polynomial z with coefficients in [-(gamma1-1), gamma1].
 *
 * @param[out] r       Output byte array.
 * @param[in]  a       Input polynomial.
 * @param[in]  gamma1  Coefficient range.
 *
 * @return             Number of bytes written.
 */
uint32_t MLDSA_PACK_polyz(uint8_t *r, const mldsa_poly *a, int32_t gamma1);

/**
 * @brief   Unpack polynomial z.
 *
 * @param[out] r       Output polynomial.
 * @param[in]  a       Input byte array.
 * @param[in]  gamma1  Coefficient range.
 *
 * @return             Number of bytes consumed.
 */
uint32_t MLDSA_PACK_unpack_polyz(mldsa_poly *r, const uint8_t *a, int32_t gamma1);

/**
 * @brief   Bit-pack polynomial w1 with coefficients fitting in
 *          ceil(log2((q-1)/(2*gamma2))) bits.
 *
 * @param[out] r       Output byte array.
 * @param[in]  a       Input polynomial.
 * @param[in]  gamma2  Low-order rounding range.
 *
 * @return             Number of bytes written.
 */
uint32_t MLDSA_PACK_polyw1(uint8_t *r, const mldsa_poly *a, int32_t gamma2);

/**
 * @brief   Pack the public key (rho || t1).
 *
 * @param[out] pk  Output byte array.
 * @param[in]  rho Seed of MLDSA_SEEDBYTES bytes.
 * @param[in]  t1  Array of k polynomials.
 * @param[in]  p   Parameter info.
 */
void MLDSA_PACK_pk(uint8_t                  *pk,
                   const uint8_t             rho[MLDSA_SEEDBYTES],
                   const mldsa_poly         *t1,
                   const MLDSA_param_info_t *p);

/**
 * @brief   Unpack the public key.
 *
 * @param[out] rho Seed output.
 * @param[out] t1  Array of k polynomials output.
 * @param[in]  pk  Packed public key.
 * @param[in]  p   Parameter info.
 */
void MLDSA_PACK_unpack_pk(uint8_t                   rho[MLDSA_SEEDBYTES],
                          mldsa_poly               *t1,
                          const uint8_t            *pk,
                          const MLDSA_param_info_t *p);

/**
 * @brief   Pack the secret key (rho || K || tr || s1 || s2 || t0).
 *
 * @param[out] sk  Output byte array.
 * @param[in]  rho Seed of MLDSA_SEEDBYTES bytes.
 * @param[in]  K   Key seed of MLDSA_SEEDBYTES bytes.
 * @param[in]  tr  Hash of MLDSA_TRBYTES bytes.
 * @param[in]  s1  Array of l polynomials.
 * @param[in]  s2  Array of k polynomials.
 * @param[in]  t0  Array of k polynomials.
 * @param[in]  p   Parameter info.
 */
void MLDSA_PACK_sk(uint8_t                  *sk,
                   const uint8_t             rho[MLDSA_SEEDBYTES],
                   const uint8_t             K[MLDSA_SEEDBYTES],
                   const uint8_t             tr[MLDSA_TRBYTES],
                   const mldsa_poly         *s1,
                   const mldsa_poly         *s2,
                   const mldsa_poly         *t0,
                   const MLDSA_param_info_t *p);

/**
 * @brief   Unpack the secret key.
 *
 * @param[out] rho Seed output.
 * @param[out] K   Key seed output.
 * @param[out] tr  Hash output.
 * @param[out] s1  Array of l polynomials output.
 * @param[out] s2  Array of k polynomials output.
 * @param[out] t0  Array of k polynomials output.
 * @param[in]  sk  Packed secret key.
 * @param[in]  p   Parameter info.
 */
void MLDSA_PACK_unpack_sk(uint8_t                   rho[MLDSA_SEEDBYTES],
                          uint8_t                   K[MLDSA_SEEDBYTES],
                          uint8_t                   tr[MLDSA_TRBYTES],
                          mldsa_poly               *s1,
                          mldsa_poly               *s2,
                          mldsa_poly               *t0,
                          const uint8_t            *sk,
                          const MLDSA_param_info_t *p);

/**
 * @brief   Pack signature (c_tilde || z || h).
 *
 * @param[out] sig     Signature output.
 * @param[in]  ctilde  Challenge hash.
 * @param[in]  z       Array of l polynomials.
 * @param[in]  h       Array of k polynomials (hint).
 * @param[in]  p       Parameter info.
 */
void MLDSA_PACK_sig(uint8_t                  *sig,
                    const uint8_t            *ctilde,
                    const mldsa_poly         *z,
                    const mldsa_poly         *h,
                    const MLDSA_param_info_t *p);

/**
 * @brief   Unpack signature.
 *
 * @param[out] ctilde Challenge hash output.
 * @param[out] z      Array of l polynomials output.
 * @param[out] h      Array of k polynomials (hint) output.
 * @param[in]  sig    Packed signature.
 * @param[in]  p      Parameter info.
 *
 * @return            0 on success, 1 if hint encoding is invalid.
 */
int MLDSA_PACK_unpack_sig(uint8_t                  *ctilde,
                          mldsa_poly               *z,
                          mldsa_poly               *h,
                          const uint8_t            *sig,
                          const MLDSA_param_info_t *p);

#endif /* CX_MLDSA_PACKING_H */
