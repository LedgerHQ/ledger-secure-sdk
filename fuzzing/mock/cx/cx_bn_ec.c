/* Big-number and EC point mocks.
 * Strong overrides for generated syscall stubs used by lib_cxng.
 */

#include <string.h>
#include <stdbool.h>

#include "ox_bn.h"
#include "ox_ec.h"

#define BN_POOL_SIZE 16
#define BN_MAX_BYTES 64

typedef struct {
    uint8_t data[BN_MAX_BYTES];
    size_t  size;
} bn_slot_t;

static bn_slot_t bn_pool[BN_POOL_SIZE];
static int       bn_next_slot;

static int bn_slot_index(cx_bn_t x)
{
    int idx = (int) x - 1;
    if (idx < 0 || idx >= BN_POOL_SIZE) {
        return -1;
    }
    return idx;
}

cx_err_t cx_bn_lock(size_t   word_nbytes __attribute__((unused)),
                    uint32_t flags __attribute__((unused)))
{
    memset(bn_pool, 0, sizeof(bn_pool));
    bn_next_slot = 0;
    return CX_OK;
}

uint32_t cx_bn_unlock(void)
{
    return CX_OK;
}

cx_err_t cx_bn_alloc(cx_bn_t *x, size_t size)
{
    if (x == NULL) {
        return CX_INVALID_PARAMETER;
    }
    if (bn_next_slot >= BN_POOL_SIZE) {
        return CX_INTERNAL_ERROR;
    }

    int slot = bn_next_slot++;
    memset(bn_pool[slot].data, 0, BN_MAX_BYTES);
    bn_pool[slot].size = size < BN_MAX_BYTES ? size : BN_MAX_BYTES;
    *x                 = (cx_bn_t) (slot + 1);
    return CX_OK;
}

cx_err_t cx_bn_alloc_init(cx_bn_t *x, size_t nbytes, const uint8_t *value, size_t value_nbytes)
{
    cx_err_t err = cx_bn_alloc(x, nbytes);
    if (err != CX_OK) {
        return err;
    }

    int idx = bn_slot_index(*x);
    if (idx < 0) {
        return CX_INTERNAL_ERROR;
    }

    if (value != NULL && value_nbytes > 0) {
        size_t copy = value_nbytes < bn_pool[idx].size ? value_nbytes : bn_pool[idx].size;
        memcpy(bn_pool[idx].data, value, copy);
    }
    return CX_OK;
}

cx_err_t cx_bn_destroy(cx_bn_t *x)
{
    if (x != NULL) {
        *x = 0;
    }
    return CX_OK;
}

cx_err_t cx_bn_init(cx_bn_t x, const uint8_t *value, size_t value_len)
{
    int idx = bn_slot_index(x);
    if (idx < 0) {
        return CX_INVALID_PARAMETER;
    }

    memset(bn_pool[idx].data, 0, BN_MAX_BYTES);
    if (value != NULL && value_len > 0) {
        size_t copy = value_len < bn_pool[idx].size ? value_len : bn_pool[idx].size;
        memcpy(bn_pool[idx].data, value, copy);
    }
    return CX_OK;
}

cx_err_t cx_bn_copy(cx_bn_t a, const cx_bn_t b)
{
    int ia = bn_slot_index(a);
    int ib = bn_slot_index(b);
    if (ia < 0 || ib < 0) {
        return CX_INVALID_PARAMETER;
    }

    memcpy(bn_pool[ia].data, bn_pool[ib].data, BN_MAX_BYTES);
    bn_pool[ia].size = bn_pool[ib].size;
    return CX_OK;
}

cx_err_t cx_bn_set_u32(cx_bn_t x, uint32_t n)
{
    int idx = bn_slot_index(x);
    if (idx < 0) {
        return CX_INVALID_PARAMETER;
    }

    memset(bn_pool[idx].data, 0, BN_MAX_BYTES);
    size_t s = bn_pool[idx].size;
    if (s >= 4) {
        bn_pool[idx].data[s - 4] = (uint8_t) (n >> 24);
        bn_pool[idx].data[s - 3] = (uint8_t) (n >> 16);
        bn_pool[idx].data[s - 2] = (uint8_t) (n >> 8);
        bn_pool[idx].data[s - 1] = (uint8_t) (n);
    }
    return CX_OK;
}

cx_err_t cx_bn_export(const cx_bn_t x, uint8_t *bytes, size_t nbytes)
{
    if (bytes == NULL) {
        return CX_OK;
    }

    int idx = bn_slot_index(x);
    if (idx < 0) {
        memset(bytes, 0, nbytes);
        return CX_OK;
    }

    size_t stored = bn_pool[idx].size;
    if (nbytes <= stored) {
        memcpy(bytes, bn_pool[idx].data, nbytes);
    }
    else {
        memset(bytes, 0, nbytes);
        memcpy(bytes, bn_pool[idx].data, stored);
    }
    return CX_OK;
}

cx_err_t cx_bn_cmp(const cx_bn_t a, const cx_bn_t b, int *diff)
{
    if (diff == NULL) {
        return CX_INVALID_PARAMETER;
    }

    int ia = bn_slot_index(a);
    int ib = bn_slot_index(b);
    if (ia < 0 || ib < 0) {
        *diff = 0;
        return CX_OK;
    }

    size_t len = bn_pool[ia].size > bn_pool[ib].size ? bn_pool[ia].size : bn_pool[ib].size;
    int    r   = memcmp(bn_pool[ia].data, bn_pool[ib].data, len);
    *diff      = r < 0 ? -1 : (r > 0 ? 1 : 0);
    return CX_OK;
}

cx_err_t cx_bn_cmp_u32(const cx_bn_t a, uint32_t b, int *diff)
{
    if (diff == NULL) {
        return CX_INVALID_PARAMETER;
    }

    int ia = bn_slot_index(a);
    if (ia < 0) {
        *diff = 1;
        return CX_OK;
    }

    uint8_t b_bytes[BN_MAX_BYTES];
    memset(b_bytes, 0, BN_MAX_BYTES);
    size_t s = bn_pool[ia].size;
    if (s >= 4) {
        b_bytes[s - 4] = (uint8_t) (b >> 24);
        b_bytes[s - 3] = (uint8_t) (b >> 16);
        b_bytes[s - 2] = (uint8_t) (b >> 8);
        b_bytes[s - 1] = (uint8_t) (b);
    }

    int r = memcmp(bn_pool[ia].data, b_bytes, s);
    *diff = r < 0 ? -1 : (r > 0 ? 1 : 0);
    return CX_OK;
}

cx_err_t cx_bn_tst_bit(const cx_bn_t x, uint32_t pos, bool *set)
{
    if (set == NULL) {
        return CX_INVALID_PARAMETER;
    }

    int idx = bn_slot_index(x);
    if (idx < 0) {
        *set = false;
        return CX_OK;
    }

    size_t s        = bn_pool[idx].size;
    size_t byte_pos = s - 1 - (pos / 8);
    if (byte_pos >= s) {
        *set = false;
        return CX_OK;
    }
    *set = (bn_pool[idx].data[byte_pos] & (1u << (pos % 8))) != 0;
    return CX_OK;
}

cx_err_t cx_bn_mod_add(cx_bn_t r __attribute__((unused)),
                       cx_bn_t a __attribute__((unused)),
                       cx_bn_t b __attribute__((unused)),
                       cx_bn_t m __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_bn_mod_sub(cx_bn_t r __attribute__((unused)),
                       cx_bn_t a __attribute__((unused)),
                       cx_bn_t b __attribute__((unused)),
                       cx_bn_t m __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_bn_mod_mul(cx_bn_t r __attribute__((unused)),
                       cx_bn_t a __attribute__((unused)),
                       cx_bn_t b __attribute__((unused)),
                       cx_bn_t m __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_bn_mod_invert_nprime(cx_bn_t r __attribute__((unused)),
                                 cx_bn_t a __attribute__((unused)),
                                 cx_bn_t m __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_ecpoint_alloc(cx_ecpoint_t *p __attribute__((unused)),
                          cx_curve_t    curve __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_ecpoint_destroy(cx_ecpoint_t *p __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_ecpoint_init(cx_ecpoint_t  *p __attribute__((unused)),
                         const uint8_t *x __attribute__((unused)),
                         size_t         x_len __attribute__((unused)),
                         const uint8_t *y __attribute__((unused)),
                         size_t         y_len __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_ecpoint_scalarmul(cx_ecpoint_t  *p __attribute__((unused)),
                              const uint8_t *k __attribute__((unused)),
                              size_t         k_len __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_ecpoint_add(cx_ecpoint_t       *r __attribute__((unused)),
                        const cx_ecpoint_t *p __attribute__((unused)),
                        const cx_ecpoint_t *q __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_ecpoint_neg(cx_ecpoint_t *p __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_ecpoint_cmp(const cx_ecpoint_t *p __attribute__((unused)),
                        const cx_ecpoint_t *q __attribute__((unused)),
                        bool               *is_equal)
{
    if (is_equal != NULL) {
        *is_equal = false;
    }
    return CX_OK;
}

cx_err_t cx_ecpoint_compress(const cx_ecpoint_t *p __attribute__((unused)),
                             uint8_t            *xy_compressed,
                             size_t              xy_compressed_len,
                             uint32_t           *sign)
{
    if (xy_compressed != NULL) {
        memset(xy_compressed, 0xAA, xy_compressed_len);
    }
    if (sign != NULL) {
        *sign = 0;
    }
    return CX_OK;
}

cx_err_t cx_ecpoint_decompress(cx_ecpoint_t  *p __attribute__((unused)),
                               const uint8_t *xy_compressed __attribute__((unused)),
                               size_t         xy_compressed_len __attribute__((unused)),
                               uint32_t       sign __attribute__((unused)))
{
    return CX_OK;
}

cx_err_t cx_ecdomain_parameter(cx_curve_t           cv __attribute__((unused)),
                               cx_curve_dom_param_t id __attribute__((unused)),
                               uint8_t             *p,
                               uint32_t             p_len)
{
    if (p != NULL) {
        memset(p, 0xFF, p_len);
    }
    return CX_OK;
}

cx_err_t cx_ecdomain_generator_bn(cx_curve_t    cv __attribute__((unused)),
                                  cx_ecpoint_t *p __attribute__((unused)))
{
    return CX_OK;
}
