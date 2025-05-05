#ifndef __weak
#define __weak __attribute__((weak))
#endif
#include "ox_aes.h"
#define SYSCALL_STUB

#if defined(HAVE_BOLOS)
#include "bolos_privileged_ux.h"
#include "cert_privileged.h"
#include "endorsement_privileged.h"
#endif  // HAVE_BOLOS

#include "bolos_target.h"
#include "exceptions.h"
#include "lcx_aes.h"
#include "lcx_eddsa.h"
#include "lcx_wrappers.h"
#include "cx_errors.h"
#include "os_task.h"
#include "os_memory.h"
#include "os_registry.h"
#include "os_ux.h"
#ifdef HAVE_SE_TOUCH
#include "os_io.h"
#endif  // HAVE_SE_TOUCH
#include "ox_ec.h"
#include "ox_bn.h"
#include "syscalls.h"
#if defined(HAVE_LANGUAGE_PACK)
#include "ux.h"
#endif  // defined(HAVE_LANGUAGE_PACK)
#ifdef HAVE_NBGL
#include "nbgl_types.h"
#include "nbgl_fonts.h"
#include "os_pic.h"
#endif
#if defined(HAVE_VSS)
#include "ox_vss.h"
#endif  // HAVE_VSS
#include "os_seed.h"
#include "ox_crc.h"
#include "os_endorsement.h"
#include <string.h>

unsigned int SVC_Call(unsigned int syscall_id, void *parameters);
unsigned int SVC_cx_call(unsigned int syscall_id, unsigned int *parameters);

__weak unsigned int get_api_level(void)
{
    return 0;
}

__weak void halt(void)
{
    return;
}

#ifdef HAVE_NBGL
__weak void nbgl_frontDrawRect(nbgl_area_t *area)
{
    return;
}

__weak void nbgl_frontDrawHorizontalLine(nbgl_area_t *area, uint8_t mask, color_t lineColor)
{
    return;
}

__weak void nbgl_frontDrawImage(nbgl_area_t          *area,
                                uint8_t              *buffer,
                                nbgl_transformation_t transformation,
                                nbgl_color_map_t      colorMap)
{
    return;
}

__weak void nbgl_frontDrawImageRle(nbgl_area_t *area,
                                   uint8_t     *buffer,
                                   uint32_t     buffer_len,
                                   color_t      fore_color,
                                   uint8_t      nb_skipped_bytes)
{
    return;
}

#ifdef NBGL_MASKING
__weak void nbgl_frontControlAreaMasking(uint8_t mask_index, nbgl_area_t *masked_area_or_null)
{
    return;
}
#endif  // NBGL_MASKING

__weak void nbgl_frontDrawImageFile(nbgl_area_t     *area,
                                    uint8_t         *buffer,
                                    nbgl_color_map_t colorMap,
                                    uint8_t         *optional_uzlib_work_buffer)
{
    return;
}

__weak void nbgl_frontRefreshArea(nbgl_area_t        *area,
                                  nbgl_refresh_mode_t mode,
                                  nbgl_post_refresh_t post_refresh)
{
    return;
}

__weak void nbgl_sideDrawRect(nbgl_area_t *area)
{
    return;
}

__weak void nbgl_sideDrawHorizontalLine(nbgl_area_t *area, uint8_t mask, color_t lineColor)
{
    return;
}

__weak void nbgl_sideDrawImage(nbgl_area_t          *area,
                               uint8_t              *buffer,
                               nbgl_transformation_t transformation,
                               nbgl_color_map_t      colorMap)
{
    return;
}

__weak void nbgl_sideRefreshArea(nbgl_area_t *area, nbgl_post_refresh_t post_refresh)
{
    return;
}

__weak void nbgl_screen_reinit(void)
{
    return;
}

#ifdef HAVE_SE_EINK_DISPLAY
__weak void nbgl_wait_pipeline(void)
{
    return;
}
#endif  // HAVE_SE_EINK_DISPLAY

#ifdef HAVE_STAX_DISPLAY_FAST_MODE
__weak void nbgl_screen_update_temperature(uint8_t temp_degrees)
{
    return;
}
#endif  // HAVE_STAX_DISPLAY_FAST_MODE

#ifdef HAVE_STAX_CONFIG_DISPLAY_FAST_MODE
__weak void nbgl_screen_config_fast_mode(uint8_t fast_mode_setting)
{
    return;
}
#endif  // HAVE_STAX_CONFIG_DISPLAY_FAST_MODE

#endif

__weak void nvm_write(void *dst_adr, void *src_adr, unsigned int src_len)
{
    return;
}

__weak void nvm_erase(void *dst_adr, unsigned int len)
{
    return;
}

__weak cx_err_t cx_aes_set_key_hw(const cx_aes_key_t *keys, uint32_t mode)
{
    return 0x00000000;
}

__weak cx_err_t cx_aes_reset_hw(void)
{
    return 0x00000000;
}

__weak cx_err_t cx_aes_block_hw(const unsigned char *inblock, unsigned char *outblock)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_lock(size_t word_nbytes, uint32_t flags)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_alloc(cx_bn_t *x, size_t nbytes)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_alloc_init(cx_bn_t       *x,
                                 size_t         nbytes,
                                 const uint8_t *value,
                                 size_t         value_nbytes)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_destroy(cx_bn_t *x)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_nbytes(const cx_bn_t x, size_t *nbytes)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_init(cx_bn_t x, const uint8_t *value, size_t value_nbytes)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_rand(cx_bn_t x)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_copy(cx_bn_t a, const cx_bn_t b)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_set_u32(cx_bn_t x, uint32_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_get_u32(const cx_bn_t x, uint32_t *n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_export(const cx_bn_t x, uint8_t *bytes, size_t nbytes)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_cmp(const cx_bn_t a, const cx_bn_t b, int *diff)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_cmp_u32(const cx_bn_t a, uint32_t b, int *diff)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_is_odd(const cx_bn_t n, bool *odd)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_xor(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_or(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_and(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_tst_bit(const cx_bn_t x, uint32_t pos, bool *set)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_set_bit(cx_bn_t x, uint32_t pos)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_clr_bit(cx_bn_t x, uint32_t pos)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_shr(cx_bn_t x, uint32_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_shl(cx_bn_t x, uint32_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_cnt_bits(cx_bn_t n, uint32_t *nbits)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_add(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_sub(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_mul(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_mod_add(cx_bn_t r, const cx_bn_t a, const cx_bn_t b, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_mod_sub(cx_bn_t r, const cx_bn_t a, const cx_bn_t b, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_mod_mul(cx_bn_t r, const cx_bn_t a, const cx_bn_t b, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_reduce(cx_bn_t r, const cx_bn_t d, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_mod_sqrt(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_n, uint32_t sign)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_mod_pow_bn(cx_bn_t r, const cx_bn_t a, const cx_bn_t e, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t
cx_bn_mod_pow(cx_bn_t r, const cx_bn_t a, const uint8_t *e, uint32_t e_len, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t
cx_bn_mod_pow2(cx_bn_t r, const cx_bn_t a, const uint8_t *e, uint32_t e_len, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_mod_invert_nprime(cx_bn_t r, const cx_bn_t a, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_mod_u32_invert(cx_bn_t r, uint32_t a, cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_is_prime(const cx_bn_t n, bool *prime)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_next_prime(cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_bn_rng(cx_bn_t r, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t
cx_bn_gf2_n_mul(cx_bn_t r, const cx_bn_t a, const cx_bn_t b, const cx_bn_t n, const cx_bn_t h)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_alloc(cx_bn_mont_ctx_t *ctx, size_t length)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_init(cx_bn_mont_ctx_t *ctx, const cx_bn_t n)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_init2(cx_bn_mont_ctx_t *ctx, const cx_bn_t n, const cx_bn_t h)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_to_montgomery(cx_bn_t x, const cx_bn_t z, const cx_bn_mont_ctx_t *ctx)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_from_montgomery(cx_bn_t z, const cx_bn_t x, const cx_bn_mont_ctx_t *ctx)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_mul(cx_bn_t                 r,
                            const cx_bn_t           a,
                            const cx_bn_t           b,
                            const cx_bn_mont_ctx_t *ctx)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_pow(cx_bn_t                 r,
                            const cx_bn_t           a,
                            const uint8_t          *e,
                            uint32_t                e_len,
                            const cx_bn_mont_ctx_t *ctx)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_pow_bn(cx_bn_t                 r,
                               const cx_bn_t           a,
                               const cx_bn_t           e,
                               const cx_bn_mont_ctx_t *ctx)
{
    return 0x00000000;
}

__weak cx_err_t cx_mont_invert_nprime(cx_bn_t r, const cx_bn_t a, const cx_bn_mont_ctx_t *ctx)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecdomain_size(cx_curve_t cv, size_t *length)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecdomain_parameters_length(cx_curve_t cv, size_t *length)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecdomain_parameter(cx_curve_t           cv,
                                      cx_curve_dom_param_t id,
                                      uint8_t             *p,
                                      uint32_t             p_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecdomain_parameter_bn(cx_curve_t cv, cx_curve_dom_param_t id, cx_bn_t p)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecdomain_generator(cx_curve_t cv, uint8_t *Gx, uint8_t *Gy, size_t len)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecdomain_generator_bn(cx_curve_t cv, cx_ecpoint_t *P)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_alloc(cx_ecpoint_t *P, cx_curve_t cv)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_destroy(cx_ecpoint_t *P)
{
    return 0x00000000;
}

__weak cx_err_t
cx_ecpoint_init(cx_ecpoint_t *P, const uint8_t *x, size_t x_len, const uint8_t *y, size_t y_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_init_bn(cx_ecpoint_t *P, const cx_bn_t x, const cx_bn_t y)
{
    return 0x00000000;
}

__weak cx_err_t
cx_ecpoint_export(const cx_ecpoint_t *P, uint8_t *x, size_t x_len, uint8_t *y, size_t y_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_export_bn(const cx_ecpoint_t *P, cx_bn_t *x, cx_bn_t *y)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_compress(const cx_ecpoint_t *P,
                                    uint8_t            *xy_compressed,
                                    size_t              xy_compressed_len,
                                    uint32_t           *sign)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_decompress(cx_ecpoint_t  *P,
                                      const uint8_t *xy_compressed,
                                      size_t         xy_compressed_len,
                                      uint32_t       sign)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_add(cx_ecpoint_t *R, const cx_ecpoint_t *P, const cx_ecpoint_t *Q)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_neg(cx_ecpoint_t *P)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_scalarmul(cx_ecpoint_t *P, const uint8_t *k, size_t k_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_scalarmul_bn(cx_ecpoint_t *P, const cx_bn_t bn_k)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_rnd_scalarmul(cx_ecpoint_t *P, const uint8_t *k, size_t k_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_rnd_scalarmul_bn(cx_ecpoint_t *P, const cx_bn_t bn_k)
{
    return 0x00000000;
}

#ifdef HAVE_FIXED_SCALAR_LENGTH
__weak cx_err_t cx_ecpoint_rnd_fixed_scalarmul(cx_ecpoint_t *P, const uint8_t *k, size_t k_len)
{
    return 0x00000000;
}
#endif  // HAVE_FIXED_SCALAR_LENGTH

__weak cx_err_t cx_ecpoint_double_scalarmul(cx_ecpoint_t  *R,
                                            cx_ecpoint_t  *P,
                                            cx_ecpoint_t  *Q,
                                            const uint8_t *k,
                                            size_t         k_len,
                                            const uint8_t *r,
                                            size_t         r_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_double_scalarmul_bn(cx_ecpoint_t *R,
                                               cx_ecpoint_t *P,
                                               cx_ecpoint_t *Q,
                                               const cx_bn_t bn_k,
                                               const cx_bn_t bn_r)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_cmp(const cx_ecpoint_t *P, const cx_ecpoint_t *Q, bool *is_equal)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_is_on_curve(const cx_ecpoint_t *R, bool *is_on_curve)
{
    return 0x00000000;
}

__weak cx_err_t cx_ecpoint_is_at_infinity(const cx_ecpoint_t *R, bool *is_infinite)
{
    return 0x00000000;
}

#ifdef HAVE_X25519
__weak cx_err_t cx_ecpoint_x25519(const cx_bn_t u, const uint8_t *k, size_t k_len)
{
    return 0x00000000;
}
#endif  // HAVE_X25519

#ifdef HAVE_X448
__weak cx_err_t cx_ecpoint_x448(const cx_bn_t u, const uint8_t *k, size_t k_len)
{
    return 0x00000000;
}
#endif  // HAVE_X448

#ifdef HAVE_BLS
__weak cx_err_t ox_bls12381_sign(const cx_ecfp_384_private_key_t *key,
                                 const uint8_t                   *message,
                                 size_t                           message_len,
                                 uint8_t                         *signature,
                                 size_t                           signature_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_bls12381_key_gen(uint8_t                    mode,
                                    const uint8_t             *secret,
                                    size_t                     secret_len,
                                    const uint8_t             *salt,
                                    size_t                     salt_len,
                                    uint8_t                   *key_info,
                                    size_t                     key_info_len,
                                    cx_ecfp_384_private_key_t *private_key,
                                    uint8_t                   *public_key,
                                    size_t                     public_key_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_hash_to_field(const uint8_t *msg,
                                 size_t         msg_len,
                                 const uint8_t *dst,
                                 size_t         dst_len,
                                 uint8_t       *hash,
                                 size_t         hash_len)
{
    return 0x00000000;
}

__weak cx_err_t cx_bls12381_aggregate(const uint8_t *in,
                                      size_t         in_len,
                                      bool           first,
                                      uint8_t       *aggregated_signature,
                                      size_t         signature_len)
{
    return 0x00000000;
}
#endif  // HAVE_BLS

#if defined(HAVE_VSS)
__weak cx_err_t cx_vss_generate_shares(cx_vss_share_t      *shares,
                                       cx_vss_commitment_t *commits,
                                       const uint8_t       *point,
                                       size_t               point_len,
                                       const uint8_t       *seed,
                                       size_t               seed_len,
                                       const uint8_t       *secret,
                                       size_t               secret_len,
                                       uint8_t              shares_number,
                                       uint8_t              threshold)
{
    return 0x00000000;
}

__weak cx_err_t cx_vss_combine_shares(uint8_t        *secret,
                                      size_t          secret_len,
                                      cx_vss_share_t *shares,
                                      uint8_t         threshold)
{
    return 0x00000000;
}

__weak cx_err_t cx_vss_verify_commits(cx_vss_commitment_t *commitments,
                                      uint8_t              threshold,
                                      cx_vss_commitment_t *share_commitment,
                                      uint32_t             share_index,
                                      bool                *verified)
{
    return 0x00000000;
}
#endif  // HAVE_VSS

__weak cx_err_t cx_get_random_bytes(void *buffer, size_t len)
{
    return 0x00000000;
}

__weak void cx_trng_get_random_data(uint8_t *buf, size_t size)
{
    return;
}

__weak void os_perso_erase_all(void)
{
    return;
}

__weak void os_perso_set_seed(unsigned int   identity,
                              unsigned int   algorithm,
                              unsigned char *seed,
                              unsigned int   length)
{
    return;
}

__weak void os_perso_derive_and_set_seed(unsigned char identity,
                                         const char   *prefix,
                                         unsigned int  prefix_length,
                                         const char   *passphrase,
                                         unsigned int  passphrase_length,
                                         const char   *words,
                                         unsigned int  words_length)
{
    return;
}

#if defined(HAVE_VAULT_RECOVERY_ALGO)
__weak void os_perso_derive_and_prepare_seed(const char  *words,
                                             unsigned int words_length,
                                             uint8_t     *vault_recovery_work_buffer)
{
    return;
}

__weak void os_perso_derive_and_xor_seed(uint8_t *vault_recovery_work_buffer)
{
    return;
}

#endif  // HAVE_VAULT_RECOVERY_ALGO

__weak void os_perso_master_seed(uint8_t *master_seed, size_t length, os_action_t action)
{
    return;
}

#if defined(HAVE_RECOVER)
__weak void os_perso_recover_state(uint8_t *state, os_action_t action)
{
    return;
}

#endif  // HAVE_RECOVER

__weak void os_perso_set_words(const unsigned char *words, unsigned int length)
{
    return;
}

__weak void os_perso_finalize(void)
{
    return;
}

__weak bolos_bool_t os_perso_isonboarded(void)
{
    return 0xaa;
}

__weak void os_perso_set_onboarding_status(unsigned int state,
                                           unsigned int count,
                                           unsigned int total)
{
    return;
}

__weak void os_set_ux_time_ms(unsigned int ux_ms)
{
    return;
}

__weak void os_perso_derive_node_bip32(cx_curve_t          curve,
                                       const unsigned int *path,
                                       unsigned int        pathLength,
                                       unsigned char      *privateKey,
                                       unsigned char      *chain)
{
    return;
}

__weak void os_perso_derive_node_with_seed_key(unsigned int        mode,
                                               cx_curve_t          curve,
                                               const unsigned int *path,
                                               unsigned int        pathLength,
                                               unsigned char      *privateKey,
                                               unsigned char      *chain,
                                               unsigned char      *seed_key,
                                               unsigned int        seed_key_length)
{
    return;
}

__weak void os_perso_derive_eip2333(cx_curve_t          curve,
                                    const unsigned int *path,
                                    unsigned int        pathLength,
                                    unsigned char      *privateKey)
{
    return;
}

#if defined(HAVE_SEED_COOKIE)
__weak bolos_bool_t os_perso_seed_cookie(unsigned char *seed_cookie)
{
    return 0xaa;
}
#endif  // HAVE_SEED_COOKIE

#if defined(HAVE_LEDGER_PKI)
__weak bolos_err_t os_pki_load_certificate(uint8_t                   expected_key_usage,
                                           uint8_t                  *certificate,
                                           size_t                    certificate_len,
                                           uint8_t                  *trusted_name,
                                           size_t                   *trusted_name_len,
                                           cx_ecfp_384_public_key_t *public_key)
{
    return 0xaa;
}

__weak bool os_pki_verify(uint8_t *descriptor_hash,
                          size_t   descriptor_hash_len,
                          uint8_t *signature,
                          size_t   signature_len)
{
    return true;
}

__weak bolos_err_t os_pki_get_info(uint8_t                  *key_usage,
                                   uint8_t                  *trusted_name,
                                   size_t                   *trusted_name_len,
                                   cx_ecfp_384_public_key_t *public_key)
{
    return 0xaa;
}
#endif  // HAVE_LEDGER_PKI

__weak bolos_err_t ENDORSEMENT_get_code_hash(uint8_t *out_hash)
{
    return 0xaa;
}

__weak bolos_err_t ENDORSEMENT_get_public_key(ENDORSEMENT_slot_t slot,
                                              uint8_t           *out_public_key,
                                              uint8_t           *out_public_key_length)
{
    return 0xaa;
}

__weak bolos_err_t ENDORSEMENT_get_public_key_certificate(ENDORSEMENT_slot_t slot,
                                                          uint8_t           *out_buffer,
                                                          uint8_t           *out_length)
{
    return 0xaa;
}

__weak bolos_err_t ENDORSEMENT_key1_get_app_secret(uint8_t *out_secret)
{
    return 0xaa;
}

__weak bolos_err_t ENDORSEMENT_key1_sign_data(uint8_t  *data,
                                              uint32_t  data_length,
                                              uint8_t  *out_signature,
                                              uint32_t *out_signature_length)
{
    return 0xaa;
}

__weak bolos_err_t ENDORSEMENT_key1_sign_without_code_hash(uint8_t  *data,
                                                           uint32_t  data_length,
                                                           uint8_t  *out_signature,
                                                           uint32_t *out_signature_length)
{
    return 0xaa;
}

__weak bolos_err_t ENDORSEMENT_key2_derive_and_sign_data(uint8_t  *data,
                                                         uint32_t  data_length,
                                                         uint8_t  *out_signature,
                                                         uint32_t *out_signature_length)
{
    return 0xaa;
}

__weak bolos_err_t ENDORSEMENT_get_metadata(ENDORSEMENT_slot_t slot,
                                            uint8_t           *out_metadata,
                                            uint8_t           *out_metadata_length)
{
    return 0xaa;
}

__weak void os_perso_set_pin(unsigned int         identity,
                             const unsigned char *pin,
                             unsigned int         length,
                             bool                 update_crc)
{
    return;
}

__weak void os_perso_set_current_identity_pin(unsigned char *pin, unsigned int length)
{
    return;
}

__weak bolos_bool_t os_perso_is_pin_set(void)
{
    return 0xaa;
}

__weak bolos_bool_t os_global_pin_is_validated(void)
{
    return 0xaa;
}

__weak bolos_bool_t os_global_pin_check(unsigned char *pin_buffer, unsigned char pin_length)
{
    return 0xaa;
}

__weak void os_global_pin_invalidate(void)
{
    return;
}

__weak unsigned int os_global_pin_retries(void)
{
    return 0;
}

__weak unsigned int os_registry_count(void)
{
    return 0;
}

__weak void os_registry_get(unsigned int app_idx, application_t *out_application_entry)
{
    return;
}

__weak unsigned int os_ux(bolos_ux_params_t *params)
{
    return 0;
}

__weak void os_dashboard_mbx(uint32_t cmd, uint32_t param)
{
    return;
}

__weak void os_ux_set_global(uint8_t param_type, uint8_t *param, size_t param_len)
{
    return;
}

__weak void os_lib_call(unsigned int *call_parameters)
{
    return;
}

__weak void __attribute__((noreturn)) os_lib_end(void)
{
    return;
}

__weak unsigned int os_flags(void)
{
    return 0;
}

__weak unsigned int os_version(unsigned char *version, unsigned int maxlength)
{
    return 0;
}

__weak unsigned int os_serial(unsigned char *serial, unsigned int maxlength)
{
    return 0;
}

__weak unsigned int os_seph_features(void)
{
    return 0;
}

__weak unsigned int os_seph_version(unsigned char *version, unsigned int maxlength)
{
    return 0;
}

__weak unsigned int os_bootloader_version(unsigned char *version, unsigned int maxlength)
{
    return 0;
}

__weak unsigned int os_factory_setting_get(unsigned int   id,
                                           unsigned char *value,
                                           unsigned int   maxlength)
{
    return 0;
}

__weak unsigned int os_setting_get(unsigned int   setting_id,
                                   unsigned char *value,
                                   unsigned int   maxlen)
{
    return 0;
}

__weak void os_setting_set(unsigned int setting_id, unsigned char *value, unsigned int length)
{
    return;
}

__weak void os_get_memory_info(meminfo_t *meminfo)
{
    return;
}

__weak unsigned int os_registry_get_tag(unsigned int  app_idx,
                                        unsigned int *tlvoffset,
                                        unsigned int  tag,
                                        unsigned int  value_offset,
                                        void         *buffer,
                                        unsigned int  maxlength)
{
    return 0;
}

__weak unsigned int os_registry_get_current_app_tag(unsigned int   tag,
                                                    unsigned char *buffer,
                                                    unsigned int   maxlen)
{
    return 0;
}

__weak void os_registry_delete_app_and_dependees(unsigned int app_idx)
{
    return;
}

__weak void os_registry_delete_all_apps(void)
{
    return;
}

__weak void os_sched_exec(unsigned int app_idx)
{
    return;
}

__weak void __attribute__((noreturn)) os_sched_exit(bolos_task_status_t exit_code)
{
    return;
}

__weak bolos_bool_t os_sched_is_running(unsigned int task_idx)
{
    return 0xaa;
}

__weak unsigned int os_sched_create(void        *main,
                                    void        *nvram,
                                    unsigned int nvram_length,
                                    void        *ram0,
                                    unsigned int ram0_length,
                                    void        *stack,
                                    unsigned int stack_length)
{
    return 0;
}

__weak void os_sched_kill(unsigned int taskidx)
{
    return;
}

__weak void io_seph_send(const unsigned char *buffer, unsigned short length)
{
    return;
}

__weak unsigned int io_seph_is_status_sent(void)
{
    return 0;
}

__weak void nvm_write_page(unsigned int page_adr, bool force)
{
    return;
}

__weak unsigned int nvm_erase_page(unsigned int page_adr)
{
    return 0;
}

__weak bolos_task_status_t os_sched_last_status(unsigned int task_idx)
{
    return 0x0;
}

__weak void os_sched_yield(bolos_task_status_t status)
{
    return;
}

__weak void os_sched_switch(unsigned int task_idx, bolos_task_status_t status)
{
    return;
}

__weak unsigned int os_sched_current_task(void)
{
    return 0;
}

__weak unsigned int os_allow_protected_ram(void)
{
    return 0;
}

__weak unsigned int os_deny_protected_ram(void)
{
    return 0;
}

__weak unsigned int os_allow_protected_flash(void)
{
    return 0;
}

__weak unsigned int os_deny_protected_flash(void)
{
    return 0;
}

#ifdef HAVE_CUSTOM_CA_DETAILS_IN_SETTINGS

__weak bolos_bool_t CERT_get(CERT_info_t *custom_ca)
{
    return 0xaa;
}

__weak void CERT_erase(void)
{
    return;
}
#endif  // HAVE_CUSTOM_CA_DETAILS_IN_SETTINGS

#ifdef HAVE_BOLOS
__weak bolos_err_t ENDORSEMENT_revoke_slot(ENDORSEMENT_revoke_id_t revoke_id)
{
    return 0xaa;
}
#endif  // HAVE_BOLOS

__weak unsigned int os_seph_serial(unsigned char *serial, unsigned int maxlength)
{
    return 0;
}

#ifdef HAVE_SE_SCREEN
__weak void screen_clear(void)
{
    return;
}

__weak void screen_update(void)
{
    return;
}

#ifdef HAVE_BRIGHTNESS_SYSCALL
__weak void screen_set_brightness(unsigned int percent)
{
    return;
}
#endif  // HAVE_BRIGHTNESS_SYSCALL

__weak void screen_set_keepout(unsigned int x,
                               unsigned int y,
                               unsigned int width,
                               unsigned int height)
{
    return;
}

__weak bolos_err_t bagl_hal_draw_bitmap_within_rect(int                  x,
                                                    int                  y,
                                                    unsigned int         width,
                                                    unsigned int         height,
                                                    unsigned int         color_count,
                                                    const unsigned int  *colors,
                                                    unsigned int         bit_per_pixel,
                                                    const unsigned char *bitmap,
                                                    unsigned int         bitmap_length_bits)
{
    return 0xaa;
}

__weak void bagl_hal_draw_rect(unsigned int color,
                               int          x,
                               int          y,
                               unsigned int width,
                               unsigned int height)
{
    return;
}
#endif

#ifdef HAVE_BLE
__weak void os_ux_set_status(unsigned int ux_id, unsigned int status)
{
    return;
}

__weak unsigned int os_ux_get_status(unsigned int ux_id)
{
    return 0;
}
#endif  // HAVE_BLE

#ifdef HAVE_SE_BUTTON
__weak unsigned int io_button_read(void)
{
    return 0;
}
#endif  // HAVE_SE_BUTTON

#if defined(HAVE_LANGUAGE_PACK)
__weak void list_language_packs(UX_LOC_LANGUAGE_PACK_INFO *packs)
{
    return;
}
#endif  // defined(HAVE_LANGUAGE_PACK)

#if defined(HAVE_BACKGROUND_IMG)

__weak bolos_err_t delete_background_img(void)
{
    return 0xaa;
}
#endif

#ifdef HAVE_SE_TOUCH
__weak void touch_get_last_info(io_touch_info_t *info)
{
    return;
}

__weak void touch_set_state(bool enable)
{
    return;
}

__weak uint8_t touch_exclude_borders(uint8_t excluded_borders)
{
    return 0;
}

#ifdef HAVE_TOUCH_READ_DEBUG_DATA_SYSCALL
__weak uint8_t touch_switch_debug_mode_and_read(io_touch_debug_mode_t mode,
                                                uint8_t               buffer_type,
                                                uint8_t              *read_buffer)
{
    return 0;
}
#endif

#endif  // HAVE_SE_TOUCH

#ifdef HAVE_IO_I2C
__weak void io_i2c_setmode(unsigned int speed_and_master, unsigned int address)
{
    return;
}

__weak void io_i2c_prepare(unsigned int maxlength)
{
    return;
}

__weak void io_i2c_xfer(void *buffer, unsigned int length, unsigned int flags)
{
    return;
}

#ifndef BOLOS_RELEASE
#ifdef BOLOS_DEBUG
__weak void io_i2c_dumpstate(void)
{
    return;
}

__weak void io_debug(char *chars, unsigned int len)
{
    return;
}
#endif  // BOLOS_DEBUG
#endif  // BOLOS_RELEASE
#endif  // HAVE_IO_I2C

#ifdef DEBUG_OS_STACK_CONSUMPTION
#endif  // DEBUG_OS_STACK_CONSUMPTION

#ifdef BOLOS_DEBUG
__weak void log_debug(const char *string)
{
    return;
}

__weak void log_debug_nw(const char *string)
{
    return;
}

__weak void log_debug_int(char *fmt, unsigned int i)
{
    return;
}

__weak void log_debug_int_nw(char *fmt, unsigned int i)
{
    return;
}

__weak void log_mem(unsigned int *adr, unsigned int len32)
{
    return;
}
#endif  // BOLOS_DEBUG
