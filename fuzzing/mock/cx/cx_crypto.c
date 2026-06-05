/* Crypto and personalization mocks.
 * fuzz_mock_crypto_fail toggles selected success and failure paths.
 */

#include <string.h>
#include <stdint.h>

#include "cx.h"
#include "os.h"

/* Absolution discovers this BSS global; apps can override it in domain-overrides.txt. */
uint8_t fuzz_mock_crypto_fail;

bolos_err_t os_perso_get_master_key_identifier(uint8_t *identifier, size_t identifier_length)
{
    if (identifier != NULL && identifier_length != 0) {
        memset(identifier, 0, identifier_length);
    }
    return 0;
}

void os_perso_derive_node_with_seed_key(unsigned int        mode __attribute__((unused)),
                                        cx_curve_t          curve __attribute__((unused)),
                                        const unsigned int *path __attribute__((unused)),
                                        unsigned int        path_len __attribute__((unused)),
                                        unsigned char      *privateKey,
                                        unsigned char      *chain,
                                        unsigned char      *seed_key __attribute__((unused)),
                                        unsigned int        seed_key_length __attribute__((unused)))
{
    if (privateKey != NULL) {
        memset(privateKey, 0x42, 64);
    }
    if (chain != NULL) {
        memset(chain, 0, 32);
    }
}

void os_perso_derive_node_bip32(cx_curve_t          curve __attribute__((unused)),
                                const unsigned int *path __attribute__((unused)),
                                unsigned int        path_len __attribute__((unused)),
                                unsigned char      *privateKey,
                                unsigned char      *chain)
{
    if (privateKey != NULL) {
        memset(privateKey, 0x42, 64);
    }
    if (chain != NULL) {
        memset(chain, 0, 32);
    }
}

cx_err_t cx_ecdomain_parameters_length(cx_curve_t cv __attribute__((unused)), size_t *length)
{
    if (length == NULL) {
        return CX_INVALID_PARAMETER;
    }
    *length = 32;
    return CX_OK;
}

cx_err_t bip32_derive_with_seed_init_privkey_256(unsigned int derivation_mode
                                                 __attribute__((unused)),
                                                 cx_curve_t      curve __attribute__((unused)),
                                                 const uint32_t *path __attribute__((unused)),
                                                 size_t          path_len __attribute__((unused)),
                                                 cx_ecfp_256_private_key_t *privkey,
                                                 uint8_t                   *chain_code,
                                                 unsigned char *seed __attribute__((unused)),
                                                 size_t         seed_len __attribute__((unused)))
{
    if (fuzz_mock_crypto_fail) {
        return CX_INTERNAL_ERROR;
    }
    if (privkey != NULL) {
        memset(privkey, 0, sizeof(*privkey));
        privkey->curve = CX_CURVE_256K1;
        privkey->d_len = 32;
        memset(privkey->d, 0x01, 32);
    }
    if (chain_code != NULL) {
        memset(chain_code, 0, 32);
    }
    return CX_OK;
}

cx_err_t cx_ecfp_generate_pair_no_throw(cx_curve_t             curve __attribute__((unused)),
                                        cx_ecfp_public_key_t  *pubkey,
                                        cx_ecfp_private_key_t *privkey __attribute__((unused)),
                                        bool                   keepprivate __attribute__((unused)))
{
    if (pubkey != NULL) {
        memset(pubkey, 0, sizeof(*pubkey));
        pubkey->curve = CX_CURVE_256K1;
        pubkey->W_len = 65;
        pubkey->W[0]  = 0x04;
        memset(&pubkey->W[1], 0xAA, 32);
        memset(&pubkey->W[33], 0xBB, 32);
    }
    return CX_OK;
}

cx_err_t cx_ecpoint_export(const cx_ecpoint_t *p __attribute__((unused)),
                           uint8_t            *x,
                           size_t              x_len,
                           uint8_t            *y,
                           size_t              y_len)
{
    if (x != NULL) {
        memset(x, 0xAA, x_len);
    }
    if (y != NULL) {
        memset(y, 0xBB, y_len);
    }
    return CX_OK;
}

cx_err_t cx_ecdsa_sign_no_throw(const cx_ecfp_private_key_t *key __attribute__((unused)),
                                uint32_t                     mode __attribute__((unused)),
                                cx_md_t                      hashID __attribute__((unused)),
                                const uint8_t               *hash __attribute__((unused)),
                                size_t                       hash_len __attribute__((unused)),
                                uint8_t                     *sig,
                                size_t                      *sig_len,
                                uint32_t                    *info)
{
    if (fuzz_mock_crypto_fail) {
        return CX_INTERNAL_ERROR;
    }
    static const uint8_t dummy_der[]
        = {0x30, 0x44, 0x02, 0x20, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
           0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
           0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x20, 0x02, 0x02, 0x02, 0x02,
           0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
           0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02};

    if (sig != NULL && sig_len != NULL) {
        size_t copy = sizeof(dummy_der);
        if (copy > *sig_len) {
            copy = *sig_len;
        }
        memcpy(sig, dummy_der, copy);
        *sig_len = copy;
    }
    if (info != NULL) {
        *info = CX_ECCINFO_PARITY_ODD;
    }
    return CX_OK;
}

cx_err_t cx_ecschnorr_sign_no_throw(const cx_ecfp_private_key_t *key __attribute__((unused)),
                                    uint32_t                     mode __attribute__((unused)),
                                    cx_md_t                      hashID __attribute__((unused)),
                                    const uint8_t               *msg __attribute__((unused)),
                                    size_t                       msg_len __attribute__((unused)),
                                    uint8_t                     *sig,
                                    size_t                      *sig_len)
{
    if (sig != NULL && sig_len != NULL && *sig_len >= 64) {
        memset(sig, 0x42, 64);
        *sig_len = 64;
    }
    return CX_OK;
}

cx_err_t cx_get_random_bytes(void *buffer, size_t len)
{
    if (buffer != NULL && len > 0) {
        memset(buffer, 0x42, len);
    }
    return CX_OK;
}
