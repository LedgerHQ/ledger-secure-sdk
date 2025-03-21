
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

#ifndef CX_CIPHER_H
#define CX_CIPHER_H

#include "lcx_cipher.h"

#ifdef HAVE_AES
#include "lcx_aes.h"

extern const cx_cipher_info_t cx_aes_128_info;
extern const cx_cipher_info_t cx_aes_192_info;
extern const cx_cipher_info_t cx_aes_256_info;

/** HW support */
WARN_UNUSED_RESULT cx_err_t cx_aes_set_key_hw(const cx_aes_key_t *keys, uint32_t mode);
WARN_UNUSED_RESULT cx_err_t cx_aes_block_hw(const uint8_t *inblock, uint8_t *outblock);
WARN_UNUSED_RESULT cx_err_t cx_aes_reset_hw(void);
#endif  // HAVE_AES

#endif /* CX_CIPHER_H */
