#ifdef HAVE_HMAC
#include "cx_hkdf.h"
#include "cx_hash.h"
#include "cx_hmac.h"
#include "cx_ram.h"

#include <string.h>

// HMAC-Based Key Kerivation Function defined at https://tools.ietf.org/html/rfc5869
// Used to implement the EIP-2333 for BLS12-381 key generation

void cx_hkdf_extract(const cx_md_t hash_id, const unsigned char* ikm, unsigned int ikm_len,
                     unsigned char *salt, unsigned int salt_len, unsigned char *prk) {
  cx_hmac_t             *hmac_ctx;
  const cx_hash_info_t  *hash_info;
  size_t                 md_len;

  hash_info = cx_hash_get_info(hash_id);
  md_len = hash_info->output_size;

  hmac_ctx = &G_cx.hmac;

  if (salt == NULL) {
    memset(hmac_ctx->key, 0 , md_len);
    salt = hmac_ctx->key;
    salt_len = md_len;
  }
  cx_hmac_init(hmac_ctx, hash_id, salt, salt_len);
  cx_hmac_update(hmac_ctx, ikm, ikm_len);
  cx_hmac_final(hmac_ctx, prk, &md_len);

}

void cx_hkdf_expand(const cx_md_t hash_id, const unsigned char *prk, unsigned int prk_len, unsigned char *info,
                   unsigned int info_len, unsigned char *okm, unsigned int okm_len) {
  unsigned char         i;
  unsigned int          offset = 0;
  unsigned char         T[MAX_HASH_SIZE];
  cx_hmac_t            *hmac_ctx;
  const cx_hash_info_t *hash_info;
  size_t                md_len;

  hash_info = cx_hash_get_info(hash_id);
  md_len = hash_info->output_size;

  hmac_ctx = &G_cx.hmac;
  
  for (i = 1; okm_len > 0; i++) {
    cx_hmac_init(hmac_ctx, hash_id, prk, prk_len);
    if (i > 1) {
      cx_hmac_update(hmac_ctx, T, offset);
    }
    cx_hmac_update(hmac_ctx, info, info_len);
    cx_hmac_update(hmac_ctx, &i, sizeof(i));
    cx_hmac_final(hmac_ctx, T, &md_len);
    offset = ( okm_len < md_len ) ? okm_len : md_len;
    memcpy(okm + (i-1) * md_len, T, offset);
    okm_len -= offset;
  }
}
#endif // HAVE_HMAC
