#ifndef MLKEM_POLYMAT_H
#define MLKEM_POLYMAT_H

#include "lcx_mlkem.h"
#include "cx_mlkem_polyvec.h"

/**
 * @brief   Matrix of polynomial vectors with up to MLKEM_MAX_K rows.
 */
typedef struct {
    polyvec vec[MLKEM_MAX_K];
} polymat;

/**
 * @brief   Generates a matrix of polynomials from a seed using rejection sampling.
 *
 * @param[out] a           Output polynomial matrix.
 * @param[in]  seed        Seed of MLKEM_SYMBYTES bytes.
 * @param[in]  transposed  If non-zero, generate the transposed matrix.
 * @param[in]  k           Dimension of the matrix (k x k).
 */
void MLKEM_POLYMAT_gen_matrix(polymat *a, const uint8_t *seed, int32_t transposed, uint8_t k);
void MLKEM_POLYMAT_gen_matrix_row(polyvec       *row,
                                  const uint8_t *seed,
                                  int32_t        transposed,
                                  uint8_t        i,
                                  uint8_t        k);

#endif  // MLKEM_POLYMAT_H
