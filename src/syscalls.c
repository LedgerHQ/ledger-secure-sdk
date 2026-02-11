#define SYSCALL_STUB

#if defined(HAVE_BOLOS)
#include "bolos_privileged_ux.h"
#endif  // HAVE_BOLOS

#include "bolos_target.h"
#include "exceptions.h"
#include "lcx_aes.h"
#include "lcx_eddsa.h"
#include "lcx_wrappers.h"
#include "cx_errors.h"
#include "os_task.h"
#include "os_registry.h"
#include "os_ux.h"
#include "os_io.h"
#ifdef HAVE_SE_TOUCH
#include "os_io_seph_ux.h"
#endif  // HAVE_SE_TOUCH
#include "ox_ec.h"
#include "ox_bn.h"
#include "syscalls.h"
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

unsigned int get_api_level(void)
{
    unsigned int parameters[2];
    parameters[0] = 0;
    parameters[1] = 0;
    return SVC_Call(SYSCALL_get_api_level_ID, parameters);
}

void halt(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    SVC_Call(SYSCALL_halt_ID, parameters);
    return;
}

#ifdef HAVE_NBGL
void nbgl_frontDrawRect(nbgl_area_t *area)
{
    unsigned int parameters[1];
    parameters[0] = (unsigned int) area;
    SVC_Call(SYSCALL_nbgl_front_draw_rect_ID, parameters);
    return;
}

void nbgl_frontDrawLine(nbgl_area_t *area, uint8_t dotStartIdx, color_t lineColor)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) area;
    parameters[1] = (unsigned int) dotStartIdx;
    parameters[2] = (unsigned int) lineColor;
    SVC_Call(SYSCALL_nbgl_front_draw_line_ID, parameters);
    return;
}

void nbgl_frontDrawImage(nbgl_area_t          *area,
                         uint8_t              *buffer,
                         nbgl_transformation_t transformation,
                         nbgl_color_map_t      colorMap)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) area;
    parameters[1] = (unsigned int) PIC(buffer);
    parameters[2] = (unsigned int) transformation;
    parameters[3] = (unsigned int) colorMap;
    SVC_Call(SYSCALL_nbgl_front_draw_img_ID, parameters);
    return;
}

void nbgl_frontDrawImageRle(nbgl_area_t *area,
                            uint8_t     *buffer,
                            uint32_t     buffer_len,
                            color_t      fore_color,
                            uint8_t      nb_skipped_bytes)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) area;
    parameters[1] = (unsigned int) PIC(buffer);
    parameters[2] = (unsigned int) buffer_len;
    parameters[3] = (unsigned int) fore_color;
    parameters[4] = (unsigned int) nb_skipped_bytes;
    SVC_Call(SYSCALL_nbgl_front_draw_img_rle_ID, parameters);
    return;
}

#ifdef NBGL_MASKING
void nbgl_frontControlAreaMasking(uint8_t mask_index, nbgl_area_t *masked_area_or_null)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) mask_index;
    parameters[1] = (unsigned int) masked_area_or_null;
    SVC_Call(SYSCALL_nbgl_front_control_area_masking_ID, parameters);
    return;
}
#endif  // NBGL_MASKING

void nbgl_frontDrawImageFile(nbgl_area_t     *area,
                             uint8_t         *buffer,
                             nbgl_color_map_t colorMap,
                             uint8_t         *optional_uzlib_work_buffer)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) area;
    parameters[1] = (unsigned int) PIC(buffer);
    parameters[2] = (unsigned int) colorMap;
    parameters[3] = (unsigned int) optional_uzlib_work_buffer;
    SVC_Call(SYSCALL_nbgl_front_draw_img_file_ID, parameters);
    return;
}

void nbgl_frontRefreshArea(nbgl_area_t        *area,
                           nbgl_refresh_mode_t mode,
                           nbgl_post_refresh_t post_refresh)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) area;
    parameters[1] = (unsigned int) mode;
    parameters[2] = (unsigned int) post_refresh;
    SVC_Call(SYSCALL_nbgl_front_refresh_area_ID, parameters);
    return;
}

void nbgl_sideDrawRect(nbgl_area_t *area)
{
    unsigned int parameters[1];
    parameters[0] = (unsigned int) area;
    SVC_Call(SYSCALL_nbgl_side_draw_rect_ID, parameters);
    return;
}

void nbgl_sideDrawHorizontalLine(nbgl_area_t *area, uint8_t mask, color_t lineColor)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) area;
    parameters[1] = (unsigned int) mask;
    parameters[2] = (unsigned int) lineColor;
    SVC_Call(SYSCALL_nbgl_side_draw_horizontal_line_ID, parameters);
    return;
}

void nbgl_sideDrawImage(nbgl_area_t          *area,
                        uint8_t              *buffer,
                        nbgl_transformation_t transformation,
                        nbgl_color_map_t      colorMap)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) area;
    parameters[1] = (unsigned int) buffer;
    parameters[2] = (unsigned int) transformation;
    parameters[3] = (unsigned int) colorMap;
    SVC_Call(SYSCALL_nbgl_side_draw_img_ID, parameters);
    return;
}

void nbgl_sideRefreshArea(nbgl_area_t *area, nbgl_post_refresh_t post_refresh)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) area;
    parameters[1] = (unsigned int) post_refresh;
    SVC_Call(SYSCALL_nbgl_side_refresh_area_ID, parameters);
    return;
}

void nbgl_screen_reinit(void)
{
    unsigned int parameters[1];
    parameters[0] = 0;
    SVC_Call(SYSCALL_nbgl_screen_reinit_ID, parameters);
}

#ifdef HAVE_SE_EINK_DISPLAY
void nbgl_wait_pipeline(void)
{
    unsigned int parameters[1];
    parameters[0] = 0;
    SVC_Call(SYSCALL_nbgl_wait_pipeline_ID, parameters);
}
#endif  // HAVE_SE_EINK_DISPLAY

#ifdef HAVE_STAX_DISPLAY_FAST_MODE
void nbgl_screen_update_temperature(uint8_t temp_degrees)
{
    unsigned int parameters[1];
    parameters[0] = (unsigned int) temp_degrees;
    SVC_Call(SYSCALL_nbgl_screen_update_temperature_ID, parameters);
    return;
}
#endif  // HAVE_STAX_DISPLAY_FAST_MODE

#ifdef HAVE_STAX_CONFIG_DISPLAY_FAST_MODE
void nbgl_screen_config_fast_mode(uint8_t fast_mode_setting)
{
    unsigned int parameters[1];
    parameters[0] = (unsigned int) fast_mode_setting;
    SVC_Call(SYSCALL_nbgl_screen_config_fast_mode_ID, parameters);
}
#endif  // HAVE_STAX_CONFIG_DISPLAY_FAST_MODE

#endif

void nvm_write(void *dst_adr, void *src_adr, unsigned int src_len)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) dst_adr;
    parameters[1] = (unsigned int) src_adr;
    parameters[2] = (unsigned int) src_len;
    SVC_Call(SYSCALL_nvm_write_ID, parameters);
    return;
}

void nvm_erase(void *dst_adr, unsigned int len)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) dst_adr;
    parameters[1] = (unsigned int) len;
    SVC_Call(SYSCALL_nvm_erase_ID, parameters);
    return;
}

cx_err_t cx_aes_set_key_hw(const cx_aes_key_t *keys, uint32_t mode)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) keys;
    parameters[1] = (unsigned int) mode;
    return SVC_cx_call(SYSCALL_cx_aes_set_key_hw_ID, parameters);
}

cx_err_t cx_aes_reset_hw(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return SVC_cx_call(SYSCALL_cx_aes_reset_hw_ID, parameters);
}

cx_err_t cx_aes_block_hw(const unsigned char *inblock, unsigned char *outblock)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) inblock;
    parameters[1] = (unsigned int) outblock;
    return SVC_cx_call(SYSCALL_cx_aes_block_hw_ID, parameters);
}

cx_err_t cx_bn_lock(size_t word_nbytes, uint32_t flags)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) word_nbytes;
    parameters[1] = (unsigned int) flags;
    return SVC_cx_call(SYSCALL_cx_bn_lock_ID, parameters);
}

uint32_t cx_bn_unlock(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (uint32_t) SVC_cx_call(SYSCALL_cx_bn_unlock_ID, parameters);
}

_Bool cx_bn_is_locked(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (_Bool) SVC_cx_call(SYSCALL_cx_bn_is_locked_ID, parameters);
}

cx_err_t cx_bn_alloc(cx_bn_t *x, size_t nbytes)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) nbytes;
    return SVC_cx_call(SYSCALL_cx_bn_alloc_ID, parameters);
}

cx_err_t cx_bn_alloc_init(cx_bn_t *x, size_t nbytes, const uint8_t *value, size_t value_nbytes)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) nbytes;
    parameters[2] = (unsigned int) value;
    parameters[3] = (unsigned int) value_nbytes;
    return SVC_cx_call(SYSCALL_cx_bn_alloc_init_ID, parameters);
}

cx_err_t cx_bn_destroy(cx_bn_t *x)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = 0;
    return SVC_cx_call(SYSCALL_cx_bn_destroy_ID, parameters);
}

cx_err_t cx_bn_nbytes(const cx_bn_t x, size_t *nbytes)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) nbytes;
    return SVC_cx_call(SYSCALL_cx_bn_nbytes_ID, parameters);
}

cx_err_t cx_bn_init(cx_bn_t x, const uint8_t *value, size_t value_nbytes)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) value;
    parameters[2] = (unsigned int) value_nbytes;
    return SVC_cx_call(SYSCALL_cx_bn_init_ID, parameters);
}

cx_err_t cx_bn_rand(cx_bn_t x)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = 0;
    return SVC_cx_call(SYSCALL_cx_bn_rand_ID, parameters);
}

cx_err_t cx_bn_copy(cx_bn_t a, const cx_bn_t b)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) a;
    parameters[1] = (unsigned int) b;
    return SVC_cx_call(SYSCALL_cx_bn_copy_ID, parameters);
}

cx_err_t cx_bn_set_u32(cx_bn_t x, uint32_t n)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_set_u32_ID, parameters);
}

cx_err_t cx_bn_get_u32(const cx_bn_t x, uint32_t *n)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) n;
    parameters[2] = 0;
    return SVC_cx_call(SYSCALL_cx_bn_get_u32_ID, parameters);
}

cx_err_t cx_bn_export(const cx_bn_t x, uint8_t *bytes, size_t nbytes)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) bytes;
    parameters[2] = (unsigned int) nbytes;
    return SVC_cx_call(SYSCALL_cx_bn_export_ID, parameters);
}

cx_err_t cx_bn_cmp(const cx_bn_t a, const cx_bn_t b, int *diff)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) a;
    parameters[1] = (unsigned int) b;
    parameters[2] = (unsigned int) diff;
    return SVC_cx_call(SYSCALL_cx_bn_cmp_ID, parameters);
}

cx_err_t cx_bn_cmp_u32(const cx_bn_t a, uint32_t b, int *diff)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) a;
    parameters[1] = (unsigned int) b;
    parameters[2] = (unsigned int) diff;
    return SVC_cx_call(SYSCALL_cx_bn_cmp_u32_ID, parameters);
}

cx_err_t cx_bn_is_odd(const cx_bn_t n, bool *odd)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) n;
    parameters[1] = (unsigned int) odd;
    return SVC_cx_call(SYSCALL_cx_bn_is_odd_ID, parameters);
}

cx_err_t cx_bn_xor(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    return SVC_cx_call(SYSCALL_cx_bn_xor_ID, parameters);
}

cx_err_t cx_bn_or(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    return SVC_cx_call(SYSCALL_cx_bn_or_ID, parameters);
}

cx_err_t cx_bn_and(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    return SVC_cx_call(SYSCALL_cx_bn_and_ID, parameters);
}

cx_err_t cx_bn_tst_bit(const cx_bn_t x, uint32_t pos, bool *set)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) pos;
    parameters[2] = (unsigned int) set;
    return SVC_cx_call(SYSCALL_cx_bn_tst_bit_ID, parameters);
}

cx_err_t cx_bn_set_bit(cx_bn_t x, uint32_t pos)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) pos;
    return SVC_cx_call(SYSCALL_cx_bn_set_bit_ID, parameters);
}

cx_err_t cx_bn_clr_bit(cx_bn_t x, uint32_t pos)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) pos;
    return SVC_cx_call(SYSCALL_cx_bn_clr_bit_ID, parameters);
}

cx_err_t cx_bn_shr(cx_bn_t x, uint32_t n)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_shr_ID, parameters);
}

cx_err_t cx_bn_shl(cx_bn_t x, uint32_t n)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_shl_ID, parameters);
}

cx_err_t cx_bn_cnt_bits(cx_bn_t n, uint32_t *nbits)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) n;
    parameters[1] = (unsigned int) nbits;
    return SVC_cx_call(SYSCALL_cx_bn_cnt_bits_ID, parameters);
}

cx_err_t cx_bn_add(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    return SVC_cx_call(SYSCALL_cx_bn_add_ID, parameters);
}

cx_err_t cx_bn_sub(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    return SVC_cx_call(SYSCALL_cx_bn_sub_ID, parameters);
}

cx_err_t cx_bn_mul(cx_bn_t r, const cx_bn_t a, const cx_bn_t b)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    return SVC_cx_call(SYSCALL_cx_bn_mul_ID, parameters);
}

cx_err_t cx_bn_mod_add(cx_bn_t r, const cx_bn_t a, const cx_bn_t b, const cx_bn_t n)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    parameters[3] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_mod_add_ID, parameters);
}

cx_err_t cx_bn_mod_sub(cx_bn_t r, const cx_bn_t a, const cx_bn_t b, const cx_bn_t n)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    parameters[3] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_mod_sub_ID, parameters);
}

cx_err_t cx_bn_mod_mul(cx_bn_t r, const cx_bn_t a, const cx_bn_t b, const cx_bn_t n)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    parameters[3] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_mod_mul_ID, parameters);
}

cx_err_t cx_bn_reduce(cx_bn_t r, const cx_bn_t d, const cx_bn_t n)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) d;
    parameters[2] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_reduce_ID, parameters);
}

cx_err_t cx_bn_mod_sqrt(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_n, uint32_t sign)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) bn_r;
    parameters[1] = (unsigned int) bn_a;
    parameters[2] = (unsigned int) bn_n;
    parameters[3] = (unsigned int) sign;
    return SVC_cx_call(SYSCALL_cx_bn_mod_sqrt_ID, parameters);
}

cx_err_t cx_bn_mod_pow_bn(cx_bn_t r, const cx_bn_t a, const cx_bn_t e, const cx_bn_t n)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) e;
    parameters[3] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_mod_pow_bn_ID, parameters);
}

cx_err_t cx_bn_mod_pow(cx_bn_t        r,
                       const cx_bn_t  a,
                       const uint8_t *e,
                       uint32_t       e_len,
                       const cx_bn_t  n)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) e;
    parameters[3] = (unsigned int) e_len;
    parameters[4] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_mod_pow_ID, parameters);
}

cx_err_t cx_bn_mod_pow2(cx_bn_t        r,
                        const cx_bn_t  a,
                        const uint8_t *e,
                        uint32_t       e_len,
                        const cx_bn_t  n)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) e;
    parameters[3] = (unsigned int) e_len;
    parameters[4] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_mod_pow2_ID, parameters);
}

cx_err_t cx_bn_mod_invert_nprime(cx_bn_t r, const cx_bn_t a, const cx_bn_t n)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_mod_invert_nprime_ID, parameters);
}

cx_err_t cx_bn_mod_u32_invert(cx_bn_t r, uint32_t a, cx_bn_t n)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_mod_u32_invert_ID, parameters);
}

cx_err_t cx_bn_is_prime(const cx_bn_t n, bool *prime)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) n;
    parameters[1] = (unsigned int) prime;
    return SVC_cx_call(SYSCALL_cx_bn_is_prime_ID, parameters);
}

cx_err_t cx_bn_next_prime(cx_bn_t n)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) n;
    parameters[1] = 0;
    return SVC_cx_call(SYSCALL_cx_bn_next_prime_ID, parameters);
}

cx_err_t cx_bn_rng(cx_bn_t r, const cx_bn_t n)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_bn_rng_ID, parameters);
}

cx_err_t cx_bn_gf2_n_mul(cx_bn_t       r,
                         const cx_bn_t a,
                         const cx_bn_t b,
                         const cx_bn_t n,
                         const cx_bn_t h)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    parameters[3] = (unsigned int) n;
    parameters[4] = (unsigned int) h;
    return SVC_cx_call(SYSCALL_cx_bn_gf2_n_mul_ID, parameters);
}

cx_err_t cx_mont_alloc(cx_bn_mont_ctx_t *ctx, size_t length)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) ctx;
    parameters[1] = (unsigned int) length;
    return SVC_cx_call(SYSCALL_cx_mont_alloc_ID, parameters);
}

cx_err_t cx_mont_init(cx_bn_mont_ctx_t *ctx, const cx_bn_t n)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) ctx;
    parameters[1] = (unsigned int) n;
    return SVC_cx_call(SYSCALL_cx_mont_init_ID, parameters);
}

cx_err_t cx_mont_init2(cx_bn_mont_ctx_t *ctx, const cx_bn_t n, const cx_bn_t h)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) ctx;
    parameters[1] = (unsigned int) n;
    parameters[2] = (unsigned int) h;
    return SVC_cx_call(SYSCALL_cx_mont_init2_ID, parameters);
}

cx_err_t cx_mont_to_montgomery(cx_bn_t x, const cx_bn_t z, const cx_bn_mont_ctx_t *ctx)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) z;
    parameters[2] = (unsigned int) ctx;
    return SVC_cx_call(SYSCALL_cx_mont_to_montgomery_ID, parameters);
}

cx_err_t cx_mont_from_montgomery(cx_bn_t z, const cx_bn_t x, const cx_bn_mont_ctx_t *ctx)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) z;
    parameters[1] = (unsigned int) x;
    parameters[2] = (unsigned int) ctx;
    return SVC_cx_call(SYSCALL_cx_mont_from_montgomery_ID, parameters);
}

cx_err_t cx_mont_mul(cx_bn_t r, const cx_bn_t a, const cx_bn_t b, const cx_bn_mont_ctx_t *ctx)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) b;
    parameters[3] = (unsigned int) ctx;
    return SVC_cx_call(SYSCALL_cx_mont_mul_ID, parameters);
}

cx_err_t cx_mont_pow(cx_bn_t                 r,
                     const cx_bn_t           a,
                     const uint8_t          *e,
                     uint32_t                e_len,
                     const cx_bn_mont_ctx_t *ctx)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) e;
    parameters[3] = (unsigned int) e_len;
    parameters[4] = (unsigned int) ctx;
    return SVC_cx_call(SYSCALL_cx_mont_pow_ID, parameters);
}

cx_err_t cx_mont_pow_bn(cx_bn_t r, const cx_bn_t a, const cx_bn_t e, const cx_bn_mont_ctx_t *ctx)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) e;
    parameters[3] = (unsigned int) ctx;
    return SVC_cx_call(SYSCALL_cx_mont_pow_bn_ID, parameters);
}

cx_err_t cx_mont_invert_nprime(cx_bn_t r, const cx_bn_t a, const cx_bn_mont_ctx_t *ctx)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) r;
    parameters[1] = (unsigned int) a;
    parameters[2] = (unsigned int) ctx;
    return SVC_cx_call(SYSCALL_cx_mont_invert_nprime_ID, parameters);
}

cx_err_t cx_ecdomain_size(cx_curve_t cv, size_t *length)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) cv;
    parameters[1] = (unsigned int) length;
    return SVC_cx_call(SYSCALL_cx_ecdomain_size_ID, parameters);
}

cx_err_t cx_ecdomain_parameters_length(cx_curve_t cv, size_t *length)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) cv;
    parameters[1] = (unsigned int) length;
    return SVC_cx_call(SYSCALL_cx_ecdomain_parameters_length_ID, parameters);
}

cx_err_t cx_ecdomain_parameter(cx_curve_t cv, cx_curve_dom_param_t id, uint8_t *p, uint32_t p_len)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) cv;
    parameters[1] = (unsigned int) id;
    parameters[2] = (unsigned int) p;
    parameters[3] = (unsigned int) p_len;
    return SVC_cx_call(SYSCALL_cx_ecdomain_parameter_ID, parameters);
}

cx_err_t cx_ecdomain_parameter_bn(cx_curve_t cv, cx_curve_dom_param_t id, cx_bn_t p)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) cv;
    parameters[1] = (unsigned int) id;
    parameters[2] = (unsigned int) p;
    return SVC_cx_call(SYSCALL_cx_ecdomain_parameter_bn_ID, parameters);
}

cx_err_t cx_ecdomain_generator(cx_curve_t cv, uint8_t *Gx, uint8_t *Gy, size_t len)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) cv;
    parameters[1] = (unsigned int) Gx;
    parameters[2] = (unsigned int) Gy;
    parameters[3] = (unsigned int) len;
    return SVC_cx_call(SYSCALL_cx_ecdomain_generator_ID, parameters);
}

cx_err_t cx_ecdomain_generator_bn(cx_curve_t cv, cx_ecpoint_t *P)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) cv;
    parameters[1] = (unsigned int) P;
    return SVC_cx_call(SYSCALL_cx_ecdomain_generator_bn_ID, parameters);
}

cx_err_t cx_ecpoint_alloc(cx_ecpoint_t *P, cx_curve_t cv)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) cv;
    return SVC_cx_call(SYSCALL_cx_ecpoint_alloc_ID, parameters);
}

cx_err_t cx_ecpoint_destroy(cx_ecpoint_t *P)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) P;
    parameters[1] = 0;
    return SVC_cx_call(SYSCALL_cx_ecpoint_destroy_ID, parameters);
}

cx_err_t cx_ecpoint_init(cx_ecpoint_t  *P,
                         const uint8_t *x,
                         size_t         x_len,
                         const uint8_t *y,
                         size_t         y_len)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) x;
    parameters[2] = (unsigned int) x_len;
    parameters[3] = (unsigned int) y;
    parameters[4] = (unsigned int) y_len;
    return SVC_cx_call(SYSCALL_cx_ecpoint_init_ID, parameters);
}

cx_err_t cx_ecpoint_init_bn(cx_ecpoint_t *P, const cx_bn_t x, const cx_bn_t y)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) x;
    parameters[2] = (unsigned int) y;
    return SVC_cx_call(SYSCALL_cx_ecpoint_init_bn_ID, parameters);
}

cx_err_t cx_ecpoint_export(const cx_ecpoint_t *P,
                           uint8_t            *x,
                           size_t              x_len,
                           uint8_t            *y,
                           size_t              y_len)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) x;
    parameters[2] = (unsigned int) x_len;
    parameters[3] = (unsigned int) y;
    parameters[4] = (unsigned int) y_len;
    return SVC_cx_call(SYSCALL_cx_ecpoint_export_ID, parameters);
}

cx_err_t cx_ecpoint_export_bn(const cx_ecpoint_t *P, cx_bn_t *x, cx_bn_t *y)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) x;
    parameters[2] = (unsigned int) y;
    return SVC_cx_call(SYSCALL_cx_ecpoint_export_bn_ID, parameters);
}

cx_err_t cx_ecpoint_compress(const cx_ecpoint_t *P,
                             uint8_t            *xy_compressed,
                             size_t              xy_compressed_len,
                             uint32_t           *sign)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) xy_compressed;
    parameters[2] = (unsigned int) xy_compressed_len;
    parameters[3] = (unsigned int) sign;
    return SVC_cx_call(SYSCALL_cx_ecpoint_compress_ID, parameters);
}

cx_err_t cx_ecpoint_decompress(cx_ecpoint_t  *P,
                               const uint8_t *xy_compressed,
                               size_t         xy_compressed_len,
                               uint32_t       sign)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) xy_compressed;
    parameters[2] = (unsigned int) xy_compressed_len;
    parameters[3] = (unsigned int) sign;
    return SVC_cx_call(SYSCALL_cx_ecpoint_decompress_ID, parameters);
}

cx_err_t cx_ecpoint_add(cx_ecpoint_t *R, const cx_ecpoint_t *P, const cx_ecpoint_t *Q)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) R;
    parameters[1] = (unsigned int) P;
    parameters[2] = (unsigned int) Q;
    return SVC_cx_call(SYSCALL_cx_ecpoint_add_ID, parameters);
}

cx_err_t cx_ecpoint_neg(cx_ecpoint_t *P)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) P;
    parameters[1] = 0;
    return SVC_cx_call(SYSCALL_cx_ecpoint_neg_ID, parameters);
}

cx_err_t cx_ecpoint_scalarmul(cx_ecpoint_t *P, const uint8_t *k, size_t k_len)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) k;
    parameters[2] = (unsigned int) k_len;
    return SVC_cx_call(SYSCALL_cx_ecpoint_scalarmul_ID, parameters);
}

cx_err_t cx_ecpoint_scalarmul_bn(cx_ecpoint_t *P, const cx_bn_t bn_k)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) bn_k;
    return SVC_cx_call(SYSCALL_cx_ecpoint_scalarmul_bn_ID, parameters);
}

cx_err_t cx_ecpoint_rnd_scalarmul(cx_ecpoint_t *P, const uint8_t *k, size_t k_len)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) k;
    parameters[2] = (unsigned int) k_len;
    return SVC_cx_call(SYSCALL_cx_ecpoint_rnd_scalarmul_ID, parameters);
}

cx_err_t cx_ecpoint_rnd_scalarmul_bn(cx_ecpoint_t *P, const cx_bn_t bn_k)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) bn_k;
    return SVC_cx_call(SYSCALL_cx_ecpoint_rnd_scalarmul_bn_ID, parameters);
}

#ifdef HAVE_FIXED_SCALAR_LENGTH
cx_err_t cx_ecpoint_rnd_fixed_scalarmul(cx_ecpoint_t *P, const uint8_t *k, size_t k_len)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) k;
    parameters[2] = (unsigned int) k_len;
    return SVC_cx_call(SYSCALL_cx_ecpoint_rnd_fixed_scalarmul_ID, parameters);
}
#endif  // HAVE_FIXED_SCALAR_LENGTH

cx_err_t cx_ecpoint_double_scalarmul(cx_ecpoint_t  *R,
                                     cx_ecpoint_t  *P,
                                     cx_ecpoint_t  *Q,
                                     const uint8_t *k,
                                     size_t         k_len,
                                     const uint8_t *r,
                                     size_t         r_len)
{
    unsigned int parameters[7];
    parameters[0] = (unsigned int) R;
    parameters[1] = (unsigned int) P;
    parameters[2] = (unsigned int) Q;
    parameters[3] = (unsigned int) k;
    parameters[4] = (unsigned int) k_len;
    parameters[5] = (unsigned int) r;
    parameters[6] = (unsigned int) r_len;
    return SVC_cx_call(SYSCALL_cx_ecpoint_double_scalarmul_ID, parameters);
}

cx_err_t cx_ecpoint_double_scalarmul_bn(cx_ecpoint_t *R,
                                        cx_ecpoint_t *P,
                                        cx_ecpoint_t *Q,
                                        const cx_bn_t bn_k,
                                        const cx_bn_t bn_r)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) R;
    parameters[1] = (unsigned int) P;
    parameters[2] = (unsigned int) Q;
    parameters[3] = (unsigned int) bn_k;
    parameters[4] = (unsigned int) bn_r;
    return SVC_cx_call(SYSCALL_cx_ecpoint_double_scalarmul_bn_ID, parameters);
}

cx_err_t cx_ecpoint_cmp(const cx_ecpoint_t *P, const cx_ecpoint_t *Q, bool *is_equal)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) P;
    parameters[1] = (unsigned int) Q;
    parameters[2] = (unsigned int) is_equal;
    return SVC_cx_call(SYSCALL_cx_ecpoint_cmp_ID, parameters);
}

cx_err_t cx_ecpoint_is_on_curve(const cx_ecpoint_t *R, bool *is_on_curve)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) R;
    parameters[1] = (unsigned int) is_on_curve;
    return SVC_cx_call(SYSCALL_cx_ecpoint_is_on_curve_ID, parameters);
}

cx_err_t cx_ecpoint_is_at_infinity(const cx_ecpoint_t *R, bool *is_infinite)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) R;
    parameters[1] = (unsigned int) is_infinite;
    return SVC_cx_call(SYSCALL_cx_ecpoint_is_at_infinity_ID, parameters);
}

#ifdef HAVE_X25519
cx_err_t cx_ecpoint_x25519(const cx_bn_t u, const uint8_t *k, size_t k_len)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) u;
    parameters[1] = (unsigned int) k;
    parameters[2] = (unsigned int) k_len;
    return SVC_cx_call(SYSCALL_cx_ecpoint_x25519_ID, parameters);
}
#endif  // HAVE_X25519

#ifdef HAVE_X448
cx_err_t cx_ecpoint_x448(const cx_bn_t u, const uint8_t *k, size_t k_len)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) u;
    parameters[1] = (unsigned int) k;
    parameters[2] = (unsigned int) k_len;
    return SVC_cx_call(SYSCALL_cx_ecpoint_x448_ID, parameters);
}
#endif  // HAVE_X448

#ifdef HAVE_BLS
cx_err_t ox_bls12381_sign(const cx_ecfp_384_private_key_t *key,
                          const uint8_t                   *message,
                          size_t                           message_len,
                          uint8_t                         *signature,
                          size_t                           signature_len)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) key;
    parameters[1] = (unsigned int) message;
    parameters[2] = (unsigned int) message_len;
    parameters[3] = (unsigned int) signature;
    parameters[4] = (unsigned int) signature_len;
    return SVC_cx_call(SYSCALL_ox_bls12381_sign_ID, parameters);
}

cx_err_t cx_bls12381_key_gen(uint8_t                    mode,
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
    unsigned int parameters[10];
    parameters[0] = (unsigned int) mode;
    parameters[1] = (unsigned int) secret;
    parameters[2] = (unsigned int) secret_len;
    parameters[3] = (unsigned int) salt;
    parameters[4] = (unsigned int) salt_len;
    parameters[5] = (unsigned int) key_info;
    parameters[6] = (unsigned int) key_info_len;
    parameters[7] = (unsigned int) private_key;
    parameters[8] = (unsigned int) public_key;
    parameters[9] = (unsigned int) public_key_len;
    return SVC_cx_call(SYSCALL_cx_bls12381_key_gen_ID, parameters);
}

cx_err_t cx_hash_to_field(const uint8_t *msg,
                          size_t         msg_len,
                          const uint8_t *dst,
                          size_t         dst_len,
                          uint8_t       *hash,
                          size_t         hash_len)
{
    unsigned int parameters[6];
    parameters[0] = (unsigned int) msg;
    parameters[1] = (unsigned int) msg_len;
    parameters[2] = (unsigned int) dst;
    parameters[3] = (unsigned int) dst_len;
    parameters[4] = (unsigned int) hash;
    parameters[5] = (unsigned int) hash_len;
    return SVC_cx_call(SYSCALL_cx_hash_to_field_ID, parameters);
}

cx_err_t cx_bls12381_aggregate(const uint8_t *in,
                               size_t         in_len,
                               bool           first,
                               uint8_t       *aggregated_signature,
                               size_t         signature_len)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) in;
    parameters[1] = (unsigned int) in_len;
    parameters[2] = (unsigned int) first;
    parameters[3] = (unsigned int) aggregated_signature;
    parameters[4] = (unsigned int) signature_len;
    return SVC_cx_call(SYSCALL_cx_bls12381_aggregate_ID, parameters);
}
#endif  // HAVE_BLS

#if defined(HAVE_VSS)
cx_err_t cx_vss_generate_shares(cx_vss_share_t      *shares,
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
    unsigned int parameters[10];
    parameters[0] = (unsigned int) shares;
    parameters[1] = (unsigned int) commits;
    parameters[2] = (unsigned int) point;
    parameters[3] = (unsigned int) point_len;
    parameters[4] = (unsigned int) seed;
    parameters[5] = (unsigned int) seed_len;
    parameters[6] = (unsigned int) secret;
    parameters[7] = (unsigned int) secret_len;
    parameters[8] = (unsigned int) shares_number;
    parameters[9] = (unsigned int) threshold;
    return SVC_cx_call(SYSCALL_cx_vss_generate_shares_ID, parameters);
}

cx_err_t cx_vss_combine_shares(uint8_t        *secret,
                               size_t          secret_len,
                               cx_vss_share_t *shares,
                               uint8_t         threshold)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) secret;
    parameters[1] = (unsigned int) secret_len;
    parameters[2] = (unsigned int) shares;
    parameters[3] = (unsigned int) threshold;
    return SVC_cx_call(SYSCALL_cx_vss_combine_shares_ID, parameters);
}

cx_err_t cx_vss_verify_commits(cx_vss_commitment_t *commitments,
                               uint8_t              threshold,
                               cx_vss_commitment_t *share_commitment,
                               uint32_t             share_index,
                               bool                *verified)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) commitments;
    parameters[1] = (unsigned int) threshold;
    parameters[2] = (unsigned int) share_commitment;
    parameters[3] = (unsigned int) share_index;
    parameters[4] = (unsigned int) verified;
    return SVC_cx_call(SYSCALL_cx_vss_verify_commits_ID, parameters);
}
#endif  // HAVE_VSS

uint32_t cx_crc_hw(crc_type_t crc_type, uint32_t crc_state, const void *buf, size_t len)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) crc_type;
    parameters[1] = (unsigned int) crc_state;
    parameters[2] = (unsigned int) buf;
    parameters[3] = (unsigned int) len;
    return (uint32_t) SVC_cx_call(SYSCALL_cx_crc_hw_ID, parameters);
}

cx_err_t cx_get_random_bytes(void *buffer, size_t len)
{
    unsigned int parameters[2 + 2];
    parameters[0] = (unsigned int) buffer;
    parameters[1] = (unsigned int) len;
    return SVC_cx_call(SYSCALL_cx_get_random_bytes_ID, parameters);
}

void cx_trng_get_random_data(uint8_t *buf, size_t size)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) buf;
    parameters[1] = (unsigned int) size;
    SVC_cx_call(SYSCALL_cx_trng_get_random_data_ID, parameters);
}

bolos_bool_t os_perso_isonboarded(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (bolos_bool_t) SVC_Call(SYSCALL_os_perso_isonboarded_ID, parameters);
}

void os_set_ux_time_ms(unsigned int ux_ms)
{
    unsigned int parameters[1];
    parameters[0] = (unsigned int) ux_ms;
    SVC_Call(SYSCALL_os_set_ux_time_ms_ID, parameters);
    return;
}

void os_perso_derive_node_bip32(cx_curve_t          curve,
                                const unsigned int *path,
                                unsigned int        pathLength,
                                unsigned char      *privateKey,
                                unsigned char      *chain)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) curve;
    parameters[1] = (unsigned int) path;
    parameters[2] = (unsigned int) pathLength;
    parameters[3] = (unsigned int) privateKey;
    parameters[4] = (unsigned int) chain;
    SVC_Call(SYSCALL_os_perso_derive_node_bip32_ID, parameters);
    return;
}

void os_perso_derive_node_with_seed_key(unsigned int        mode,
                                        cx_curve_t          curve,
                                        const unsigned int *path,
                                        unsigned int        pathLength,
                                        unsigned char      *privateKey,
                                        unsigned char      *chain,
                                        unsigned char      *seed_key,
                                        unsigned int        seed_key_length)
{
    unsigned int parameters[8];
    parameters[0] = (unsigned int) mode;
    parameters[1] = (unsigned int) curve;
    parameters[2] = (unsigned int) path;
    parameters[3] = (unsigned int) pathLength;
    parameters[4] = (unsigned int) privateKey;
    parameters[5] = (unsigned int) chain;
    parameters[6] = (unsigned int) seed_key;
    parameters[7] = (unsigned int) seed_key_length;
    SVC_Call(SYSCALL_os_perso_derive_node_with_seed_key_ID, parameters);
    return;
}

void os_perso_derive_eip2333(cx_curve_t          curve,
                             const unsigned int *path,
                             unsigned int        pathLength,
                             unsigned char      *privateKey)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) curve;
    parameters[1] = (unsigned int) path;
    parameters[2] = (unsigned int) pathLength;
    parameters[3] = (unsigned int) privateKey;
    SVC_Call(SYSCALL_os_perso_derive_eip2333_ID, parameters);
    return;
}

bolos_err_t os_perso_get_master_key_identifier(uint8_t *identifier, size_t identifier_length)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) identifier;
    parameters[1] = (unsigned int) identifier_length;
    return (bolos_err_t) SVC_Call(SYSCALL_os_perso_get_master_key_identifier_ID, parameters);
}

#if defined(HAVE_LEDGER_PKI)
bolos_err_t os_pki_load_certificate(uint8_t                   expected_key_usage,
                                    uint8_t                  *certificate,
                                    size_t                    certificate_len,
                                    uint8_t                  *trusted_name,
                                    size_t                   *trusted_name_len,
                                    cx_ecfp_384_public_key_t *public_key)
{
    unsigned int parameters[6];
    parameters[0] = (unsigned int) expected_key_usage;
    parameters[1] = (unsigned int) certificate;
    parameters[2] = (unsigned int) certificate_len;
    parameters[3] = (unsigned int) trusted_name;
    parameters[4] = (unsigned int) trusted_name_len;
    parameters[5] = (unsigned int) public_key;
    return (bolos_err_t) SVC_Call(SYSCALL_os_pki_load_certificate_ID, parameters);
}

bool os_pki_verify(uint8_t *descriptor_hash,
                   size_t   descriptor_hash_len,
                   uint8_t *signature,
                   size_t   signature_len)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) descriptor_hash;
    parameters[1] = (unsigned int) descriptor_hash_len;
    parameters[2] = (unsigned int) signature;
    parameters[3] = (unsigned int) signature_len;
    return (bool) SVC_Call(SYSCALL_os_pki_verify_ID, parameters);
}

bolos_err_t os_pki_get_info(uint8_t                  *key_usage,
                            uint8_t                  *trusted_name,
                            size_t                   *trusted_name_len,
                            cx_ecfp_384_public_key_t *public_key)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) key_usage;
    parameters[1] = (unsigned int) trusted_name;
    parameters[2] = (unsigned int) trusted_name_len;
    parameters[3] = (unsigned int) public_key;
    return (bolos_err_t) SVC_Call(SYSCALL_os_pki_get_info_ID, parameters);
}
#endif  // HAVE_LEDGER_PKI

bolos_err_t sys_endorsement_get_code_hash(uint8_t *hash, size_t length)
{
    uint32_t parameters[2] = {0};
    parameters[0]          = (uintptr_t) hash;
    parameters[1]          = length;
    return (bolos_err_t) SVC_Call(SYSCALL_ENDORSEMENT_GET_CODE_HASH_ID, parameters);
}

bolos_err_t sys_endorsement_get_public_key(ENDORSEMENT_slot_t slot,
                                           uint8_t           *public_key,
                                           size_t            *public_key_length)
{
    uint32_t parameters[3] = {0};
    parameters[0]          = slot;
    parameters[1]          = (uintptr_t) public_key;
    parameters[2]          = (uintptr_t) public_key_length;
    return (bolos_err_t) SVC_Call(SYSCALL_ENDORSEMENT_GET_PUB_KEY_ID, parameters);
}

bolos_err_t sys_endorsement_get_public_key_signature(ENDORSEMENT_slot_t slot,
                                                     uint8_t           *sig,
                                                     size_t            *sig_length)
{
    uint32_t parameters[3] = {0};
    parameters[0]          = slot;
    parameters[1]          = (uintptr_t) sig;
    parameters[2]          = (uintptr_t) sig_length;
    return (bolos_err_t) SVC_Call(SYSCALL_ENDORSEMENT_GET_PUB_KEY_SIG_ID, parameters);
}

bolos_err_t sys_endorsement_key1_get_app_secret(uint8_t *secret, size_t length)
{
    uint32_t parameters[2] = {0};
    parameters[0]          = (uintptr_t) secret;
    parameters[1]          = length;
    return (bolos_err_t) SVC_Call(SYSCALL_ENDORSEMENT_KEY1_GET_APP_SECRET_ID, parameters);
}

bolos_err_t sys_endorsement_key1_sign_data(const uint8_t *data,
                                           size_t         data_length,
                                           uint8_t       *sig,
                                           size_t        *sig_length)
{
    uint32_t parameters[4] = {0};
    parameters[0]          = (uintptr_t) data;
    parameters[1]          = data_length;
    parameters[2]          = (uintptr_t) sig;
    parameters[3]          = (uintptr_t) sig_length;
    return (bolos_err_t) SVC_Call(SYSCALL_ENDORSEMENT_KEY1_SIGN_DATA_ID, parameters);
}

bolos_err_t sys_endorsement_key1_sign_without_code_hash(const uint8_t *data,
                                                        size_t         data_length,
                                                        uint8_t       *sig,
                                                        size_t        *sig_length)
{
    uint32_t parameters[4] = {0};
    parameters[0]          = (uintptr_t) data;
    parameters[1]          = data_length;
    parameters[2]          = (uintptr_t) sig;
    parameters[3]          = (uintptr_t) sig_length;
    return (bolos_err_t) SVC_Call(SYSCALL_ENDORSEMENT_KEY1_SIGN_WITHOUT_CODE_HASH_ID, parameters);
}

bolos_err_t sys_endorsement_key2_derive_and_sign_data(const uint8_t *data,
                                                      size_t         data_length,
                                                      uint8_t       *sig,
                                                      size_t        *sig_length)
{
    uint32_t parameters[4] = {0};
    parameters[0]          = (uintptr_t) data;
    parameters[1]          = data_length;
    parameters[2]          = (uintptr_t) sig;
    parameters[3]          = (uintptr_t) sig_length;
    return (bolos_err_t) SVC_Call(SYSCALL_ENDORSEMENT_KEY2_DERIVE_AND_SIGN_DATA_ID, parameters);
}

bolos_err_t sys_endorsement_get_metadata(ENDORSEMENT_slot_t slot,
                                         uint8_t           *metadata,
                                         size_t            *metadata_length)
{
    uint32_t parameters[3] = {0};
    parameters[0]          = slot;
    parameters[1]          = (uintptr_t) metadata;
    parameters[2]          = (uintptr_t) metadata_length;
    return (bolos_err_t) SVC_Call(SYSCALL_ENDORSEMENT_GET_METADATA_ID, parameters);
}

bolos_bool_t os_perso_is_pin_set(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (bolos_bool_t) SVC_Call(SYSCALL_os_perso_is_pin_set_ID, parameters);
}

bolos_bool_t os_global_pin_is_validated(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (bolos_bool_t) SVC_Call(SYSCALL_os_global_pin_is_validated_ID, parameters);
}

bolos_bool_t os_global_pin_check(unsigned char *pin_buffer, unsigned char pin_length)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) pin_buffer;
    parameters[1] = (unsigned int) pin_length;
    return (bolos_bool_t) SVC_Call(SYSCALL_os_global_pin_check_ID, parameters);
}

void os_global_pin_invalidate(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    SVC_Call(SYSCALL_os_global_pin_invalidate_ID, parameters);
    return;
}

unsigned int os_global_pin_retries(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_global_pin_retries_ID, parameters);
}

unsigned int os_ux(bolos_ux_params_t *params)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) params;
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_ux_ID, parameters);
}

void os_lib_call(unsigned int *call_parameters)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) call_parameters;
    parameters[1] = 0;
    SVC_Call(SYSCALL_os_lib_call_ID, parameters);
    return;
}

void __attribute__((noreturn)) os_lib_end(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    SVC_Call(SYSCALL_os_lib_end_ID, parameters);

    // The os_lib_end syscall should never return.
    // Just in case, crash the device thanks to an undefined instruction.
    // To avoid the __builtin_unreachable undefined behaviour
    asm volatile("udf #255");

    // remove the warning caused by -Winvalid-noreturn
    __builtin_unreachable();
}

unsigned int os_flags(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_flags_ID, parameters);
}

unsigned int os_version(unsigned char *version, unsigned int maxlength)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) version;
    parameters[1] = (unsigned int) maxlength;
    return (unsigned int) SVC_Call(SYSCALL_os_version_ID, parameters);
}

unsigned int os_serial(unsigned char *serial, unsigned int maxlength)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) serial;
    parameters[1] = (unsigned int) maxlength;
    return (unsigned int) SVC_Call(SYSCALL_os_serial_ID, parameters);
}

unsigned int os_seph_features(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_seph_features_ID, parameters);
}

unsigned int os_seph_version(unsigned char *version, unsigned int maxlength)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) version;
    parameters[1] = (unsigned int) maxlength;
    return (unsigned int) SVC_Call(SYSCALL_os_seph_version_ID, parameters);
}

unsigned int os_bootloader_version(unsigned char *version, unsigned int maxlength)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) version;
    parameters[1] = (unsigned int) maxlength;
    return (unsigned int) SVC_Call(SYSCALL_os_bootloader_version_ID, parameters);
}

unsigned int os_factory_setting_get(unsigned int id, unsigned char *value, unsigned int maxlength)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) id;
    parameters[1] = (unsigned int) value;
    parameters[2] = (unsigned int) maxlength;
    return (unsigned int) SVC_Call(SYSCALL_os_factory_setting_get_ID, parameters);
}

unsigned int os_setting_get(unsigned int setting_id, unsigned char *value, unsigned int maxlen)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) setting_id;
    parameters[1] = (unsigned int) value;
    parameters[2] = (unsigned int) maxlen;
    return (unsigned int) SVC_Call(SYSCALL_os_setting_get_ID, parameters);
}

void os_setting_set(unsigned int setting_id, unsigned char *value, unsigned int length)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) setting_id;
    parameters[1] = (unsigned int) value;
    parameters[2] = (unsigned int) length;
    SVC_Call(SYSCALL_os_setting_set_ID, parameters);
    return;
}

void __attribute__((noreturn)) os_sched_exit(bolos_task_status_t exit_code)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) exit_code;
    parameters[1] = 0;
    SVC_Call(SYSCALL_os_sched_exit_ID, parameters);

    // The os_sched_exit syscall should never return.
    // Just in case, crash the device thanks to an undefined instruction.
    // To avoid the __builtin_unreachable undefined behaviour
    asm volatile("udf #255");

    // remove the warning caused by -Winvalid-noreturn
    __builtin_unreachable();
}

bolos_bool_t os_sched_is_running(unsigned int task_idx)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) task_idx;
    parameters[1] = 0;
    return (bolos_bool_t) SVC_Call(SYSCALL_os_sched_is_running_ID, parameters);
}

unsigned int os_sched_create(void        *main,
                             void        *nvram,
                             unsigned int nvram_length,
                             void        *ram0,
                             unsigned int ram0_length,
                             void        *stack,
                             unsigned int stack_length)
{
    unsigned int parameters[7];
    parameters[0] = (unsigned int) main;
    parameters[1] = (unsigned int) nvram;
    parameters[2] = (unsigned int) nvram_length;
    parameters[3] = (unsigned int) ram0;
    parameters[4] = (unsigned int) ram0_length;
    parameters[5] = (unsigned int) stack;
    parameters[6] = (unsigned int) stack_length;
    return (unsigned int) SVC_Call(SYSCALL_os_sched_create_ID, parameters);
}

void os_sched_kill(unsigned int taskidx)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) taskidx;
    parameters[1] = 0;
    SVC_Call(SYSCALL_os_sched_kill_ID, parameters);
    return;
}

int os_io_seph_tx(const unsigned char *buffer, unsigned short length, unsigned int *timeout_ms)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) buffer;
    parameters[1] = (unsigned int) length;
    parameters[2] = (unsigned int) timeout_ms;
    return (int) SVC_Call(SYSCALL_os_io_seph_tx_ID, parameters);
}

int os_io_seph_se_rx_event(unsigned char *buffer,
                           unsigned short max_length,
                           unsigned int  *timeout_ms,
                           bool           check_se_event,
                           unsigned int   flags)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) buffer;
    parameters[1] = (unsigned int) max_length;
    parameters[2] = (unsigned int) timeout_ms;
    parameters[3] = (unsigned int) check_se_event;
    parameters[4] = (unsigned int) flags;
    return (int) SVC_Call(SYSCALL_os_io_seph_se_rx_event_ID, parameters);
}

__attribute((weak)) int os_io_init(os_io_init_t *init)
{
    unsigned int parameters[1];
    parameters[0] = (unsigned int) init;
    return (int) SVC_Call(SYSCALL_os_io_init_ID, parameters);
}

__attribute((weak)) int os_io_start(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (int) SVC_Call(SYSCALL_os_io_start_ID, parameters);
}

__attribute((weak)) int os_io_stop(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (int) SVC_Call(SYSCALL_os_io_stop_ID, parameters);
}

__attribute((weak)) int os_io_tx_cmd(unsigned char        type,
                                     const unsigned char *buffer,
                                     unsigned short       length,
                                     unsigned int        *timeout_ms)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) type;
    parameters[1] = (unsigned int) buffer;
    parameters[2] = (unsigned int) length;
    parameters[3] = (unsigned int) timeout_ms;
    return (int) SVC_Call(SYSCALL_os_io_tx_cmd_ID, parameters);
}

__attribute((weak)) int os_io_rx_evt(unsigned char *buffer,
                                     unsigned short buffer_max_length,
                                     unsigned int  *timeout_ms,
                                     bool           check_se_event)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) buffer;
    parameters[1] = (unsigned int) buffer_max_length;
    parameters[2] = (unsigned int) timeout_ms;
    parameters[3] = (unsigned int) check_se_event;
    return (int) SVC_Call(SYSCALL_os_io_rx_evt_ID, parameters);
}

void nvm_write_page(unsigned int page_adr, bool force)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) page_adr;
    parameters[1] = (unsigned int) force;
    SVC_Call(SYSCALL_nvm_write_page_ID, parameters);
    return;
}

unsigned int nvm_erase_page(unsigned int page_adr)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) page_adr;
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_nvm_erase_page_ID, parameters);
}

try_context_t *try_context_get(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (try_context_t *) SVC_Call(SYSCALL_try_context_get_ID, parameters);
}

try_context_t *try_context_set(try_context_t *context)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) context;
    parameters[1] = 0;
    return (try_context_t *) SVC_Call(SYSCALL_try_context_set_ID, parameters);
}

bolos_task_status_t os_sched_last_status(unsigned int task_idx)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) task_idx;
    parameters[1] = 0;
    return (bolos_task_status_t) SVC_Call(SYSCALL_os_sched_last_status_ID, parameters);
}

void os_sched_yield(bolos_task_status_t status)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) status;
    parameters[1] = 0;
    SVC_Call(SYSCALL_os_sched_yield_ID, parameters);
    return;
}

void os_sched_switch(unsigned int task_idx, bolos_task_status_t status)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) task_idx;
    parameters[1] = (unsigned int) status;
    SVC_Call(SYSCALL_os_sched_switch_ID, parameters);
    return;
}

unsigned int os_sched_current_task(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_sched_current_task_ID, parameters);
}

unsigned int os_allow_protected_ram(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_allow_protected_ram_ID, parameters);
}

unsigned int os_deny_protected_ram(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_deny_protected_ram_ID, parameters);
}

unsigned int os_allow_protected_flash(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_allow_protected_flash_ID, parameters);
}

unsigned int os_deny_protected_flash(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_deny_protected_flash_ID, parameters);
}

unsigned int os_seph_serial(unsigned char *serial, unsigned int maxlength)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) serial;
    parameters[1] = (unsigned int) maxlength;
    return (unsigned int) SVC_Call(SYSCALL_os_seph_serial_ID, parameters);
}

#ifdef HAVE_SE_SCREEN
void screen_clear(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    SVC_Call(SYSCALL_screen_clear_ID, parameters);
    return;
}

void screen_update(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    SVC_Call(SYSCALL_screen_update_ID, parameters);
    return;
}

#ifdef HAVE_BRIGHTNESS_SYSCALL
void screen_set_brightness(unsigned int percent)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) percent;
    SVC_Call(SYSCALL_screen_set_brightness_ID, parameters);
    return;
}
#endif  // HAVE_BRIGHTNESS_SYSCALL

void screen_set_keepout(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
    unsigned int parameters[4];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) y;
    parameters[2] = (unsigned int) width;
    parameters[3] = (unsigned int) height;
    SVC_Call(SYSCALL_screen_set_keepout_ID, parameters);
    return;
}

bolos_err_t bagl_hal_draw_bitmap_within_rect(int                  x,
                                             int                  y,
                                             unsigned int         width,
                                             unsigned int         height,
                                             unsigned int         color_count,
                                             const unsigned int  *colors,
                                             unsigned int         bit_per_pixel,
                                             const unsigned char *bitmap,
                                             unsigned int         bitmap_length_bits)
{
    unsigned int parameters[9];
    parameters[0] = (unsigned int) x;
    parameters[1] = (unsigned int) y;
    parameters[2] = (unsigned int) width;
    parameters[3] = (unsigned int) height;
    parameters[4] = (unsigned int) color_count;
    parameters[5] = (unsigned int) colors;
    parameters[6] = (unsigned int) bit_per_pixel;
    parameters[7] = (unsigned int) bitmap;
    parameters[8] = (unsigned int) bitmap_length_bits;
    return SVC_Call(SYSCALL_bagl_hal_draw_bitmap_within_rect_ID, parameters);
}

void bagl_hal_draw_rect(unsigned int color, int x, int y, unsigned int width, unsigned int height)
{
    unsigned int parameters[5];
    parameters[0] = (unsigned int) color;
    parameters[1] = (unsigned int) x;
    parameters[2] = (unsigned int) y;
    parameters[3] = (unsigned int) width;
    parameters[4] = (unsigned int) height;
    SVC_Call(SYSCALL_bagl_hal_draw_rect_ID, parameters);
    return;
}
#endif  // HAVE_SE_SCREEN

#ifdef HAVE_BLE
void os_ux_set_status(unsigned int ux_id, unsigned int status)
{
    unsigned int parameters[2 + 2];
    parameters[0] = (unsigned int) ux_id;
    parameters[1] = (unsigned int) status;
    SVC_Call(SYSCALL_os_ux_set_status_ID_IN, parameters);
    return;
}

unsigned int os_ux_get_status(unsigned int ux_id)
{
    unsigned int parameters[2 + 1];
    parameters[0] = (unsigned int) ux_id;
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_os_ux_get_status_ID_IN, parameters);
}
#endif  // HAVE_BLE

#ifdef HAVE_SE_BUTTON
unsigned int io_button_read(void)
{
    unsigned int parameters[2];
    parameters[1] = 0;
    return (unsigned int) SVC_Call(SYSCALL_io_button_read_ID, parameters);
}
#endif  // HAVE_SE_BUTTON

#ifdef HAVE_SE_TOUCH
void touch_get_last_info(io_touch_info_t *info)
{
    unsigned int parameters[1] = {(unsigned int) info};
    SVC_Call(SYSCALL_touch_get_last_info_ID, parameters);
}

void touch_set_state(bool enable)
{
    unsigned int parameters[1] = {(unsigned int) enable};
    SVC_Call(SYSCALL_touch_set_state_ID, parameters);
}

uint8_t touch_exclude_borders(uint8_t excluded_borders)
{
    unsigned int parameters[1] = {(unsigned int) excluded_borders};
    return (uint8_t) SVC_Call(SYSCALL_touch_exclude_borders_ID, parameters);
}

#ifdef HAVE_TOUCH_READ_DEBUG_DATA_SYSCALL
uint8_t touch_switch_debug_mode_and_read(os_io_touch_debug_mode_t mode,
                                         uint8_t                  buffer_type,
                                         uint8_t                 *read_buffer)
{
    unsigned int parameters[3];
    parameters[0] = (unsigned int) mode;
    parameters[1] = (unsigned int) buffer_type;  // Only applicable to ewd720 touch
    parameters[2] = (unsigned int) read_buffer;
    return (uint8_t) SVC_Call(SYSCALL_touch_debug_ID, parameters);
}
#endif

#endif  // HAVE_SE_TOUCH

#ifdef DEBUG_OS_STACK_CONSUMPTION
int os_stack_operations(unsigned char mode)
{
    unsigned int parameters[2];
    parameters[0] = (unsigned int) mode;
    return (unsigned int) SVC_Call(SYSCALL_os_stack_operations_ID, parameters);
}
#endif  // DEBUG_OS_STACK_CONSUMPTION
