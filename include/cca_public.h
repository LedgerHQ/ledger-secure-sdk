/**
 * @file cca_public.h
 * @brief Types and prototypes to interact with the Custom CA module from public user-land.
 */

#ifndef CCA_PUBLIC_H_
#define CCA_PUBLIC_H_

#ifdef HAVE_BOLOS_CUSTOMCA

#include "bolos_target.h"
#include "decorators.h"

/* ----------------------------------------------------------------------- */
/* -                         CUSTOM CERTIFICATE AUTHORITY                - */
/* ----------------------------------------------------------------------- */

// Verify the signature is issued from the custom certificate authority

/**
 * @brief Verify hash signature with custom certificate authority
 *
 * @param hash Hash to be verified (32 bytes length).
 * @param sign Signature to be verified
 * @param sign_length Signature length
 * @return bool
 * @retval Verification OK
 * @retval Verification not OK
 *
 */
SYSCALL unsigned int cca_verify_custom_ca(unsigned char *hash PLENGTH(32),
                                          unsigned char *sign PLENGTH(sign_length),
                                          unsigned int        sign_length);

#endif  // HAVE_BOLOS_CUSTOMCA
#endif  // CCA_PUBLIC_H_
