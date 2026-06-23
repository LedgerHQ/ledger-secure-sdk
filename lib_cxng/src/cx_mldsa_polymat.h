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
#ifndef CX_MLDSA_POLYMAT_H
#define CX_MLDSA_POLYMAT_H

#include "cx_mldsa_polyvec.h"
#include "lcx_mldsa.h"

/**
 * @brief   Expand the k x l matrix A from a seed rho using SHAKE128.
 *          Each polynomial A[i][j] is stored as mat[i].vec[j].
 *
 * @param[out] mat  Array of k polyvecl rows.
 * @param[in]  rho  Seed of MLDSA_SEEDBYTES bytes.
 * @param[in]  p    Parameter info.
 */
void MLDSA_POLYMAT_expand(mldsa_polyvecl           *mat,
                          const uint8_t             rho[MLDSA_SEEDBYTES],
                          const MLDSA_param_info_t *p);

/**
 * @brief   Matrix-vector multiply: t = A * s (in NTT domain).
 *          Both A and s must already be in NTT domain.
 *          t is computed and left in NTT domain.
 *
 * @param[out] t    Output K-vector (NTT domain).
 * @param[in]  mat  Matrix as array of k polyvecl rows (NTT domain).
 * @param[in]  s    Input L-vector (NTT domain).
 * @param[in]  p    Parameter info.
 */
void MLDSA_POLYMAT_pointwise_montgomery(mldsa_polyveck           *t,
                                        const mldsa_polyvecl     *mat,
                                        const mldsa_polyvecl     *s,
                                        const MLDSA_param_info_t *p);

/**
 * @brief   On-the-fly A expansion with matrix-vector multiply.
 *
 *          Expands A one element at a time and accumulates A*s
 *          without ever storing the full matrix.  Only a single
 *          polynomial is kept on the callee stack.
 *
 * @param[out] t    Output K-vector (NTT domain).
 * @param[in]  rho  Seed of MLDSA_SEEDBYTES bytes.
 * @param[in]  s    Input L-vector (must be in NTT domain).
 * @param[in]  p    Parameter info.
 */
void MLDSA_POLYMAT_expand_and_multiply(mldsa_polyveck           *t,
                                       const uint8_t             rho[MLDSA_SEEDBYTES],
                                       const mldsa_polyvecl     *s,
                                       const MLDSA_param_info_t *p);

#endif /* CX_MLDSA_POLYMAT_H */
