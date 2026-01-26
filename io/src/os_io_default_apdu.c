/*****************************************************************************
 *   (c) 2025 Ledger SAS.
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "exceptions.h"
#include "lcx_hash.h"
#include "lcx_sha512.h"
// #include "os_errors.h"
#include "os_utils.h"
#include "os_apdu.h"
#include "os_debug.h"
#include "os_pin.h"
#include "os_id.h"
#include "os_seed.h"
#include "os_app.h"
#include "os_pki.h"
#include "os_io_default_apdu.h"
#include "status_words.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static bolos_err_t get_version(uint8_t *buffer_out, size_t *buffer_out_length);

#if defined(HAVE_SEED_COOKIE)
// TODO: Remove seed cookie APDU handling
#include "perso_private_syscalls.h"
static bolos_err_t get_seed_cookie(uint8_t *buffer_out, size_t *buffer_out_length);
#endif  // HAVE_SEED_COOKIE
#if defined(DEBUG_OS_STACK_CONSUMPTION)
static bolos_err_t get_stack_consumption(uint8_t  mode,
                                         uint8_t *buffer_out,
                                         size_t  *buffer_out_length);
#endif  // DEBUG_OS_STACK_CONSUMPTION
#if defined(HAVE_LEDGER_PKI)
static bolos_err_t pki_load_certificate(uint8_t *buffer, size_t buffer_len, uint8_t key_usage);
#endif  // HAVE_LEDGER_PKI

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

#ifndef APPNAME
#define APPNAME "Unknown"
#endif

#ifndef APPVERSION
#define APPVERSION "Unknown"
#endif

static bolos_err_t get_version(uint8_t *buffer_out, size_t *buffer_out_length)
{
    bolos_err_t err                   = SWO_CONDITIONS_NOT_SATISFIED;
    size_t      max_buffer_out_length = *buffer_out_length;

    *buffer_out_length = 0;

#if defined(HAVE_BOLOS)
    const char *name    = "BOLOS";
    const char *version = VERSION;
#else
    const char *name    = APPNAME;
    const char *version = APPVERSION;
#endif

    size_t name_len    = strlen(name);
    size_t version_len = strlen(version);

    if (max_buffer_out_length >= (name_len + version_len + 3 + 2)) {
        buffer_out[(*buffer_out_length)++] = 1;  // format ID

        // Copy name
        buffer_out[(*buffer_out_length)++] = name_len;
        memcpy((char *) (&buffer_out[*buffer_out_length]), name, name_len);
        *buffer_out_length += name_len;

        // Copy version
        buffer_out[(*buffer_out_length)++] = version_len;
        memcpy((char *) (&buffer_out[*buffer_out_length]), version, version_len);
        *buffer_out_length += version_len;

        // Copy os flags
        buffer_out[(*buffer_out_length)++] = 1;
        buffer_out[(*buffer_out_length)++] = os_flags();

        err = SWO_SUCCESS;
    }

    return err;
}

#if defined(HAVE_SEED_COOKIE)
static bolos_err_t get_seed_cookie(uint8_t *buffer_out, size_t *buffer_out_length)
{
    bolos_err_t err                   = SWO_CONDITIONS_NOT_SATISFIED;
    size_t      max_buffer_out_length = *buffer_out_length;

    *buffer_out_length = 0;
    if (os_global_pin_is_validated() == BOLOS_UX_OK) {
        if (max_buffer_out_length >= CX_SHA512_SIZE + 4) {
            buffer_out[(*buffer_out_length)++] = 0x01;
            bolos_bool_t seed_generated        = os_perso_seed_cookie(&buffer_out[2]);
            if (seed_generated == BOLOS_TRUE) {
                buffer_out[(*buffer_out_length)++] = CX_SHA512_SIZE;
                *buffer_out_length += CX_SHA512_SIZE;
            }
            else {
                buffer_out[(*buffer_out_length)++] = 0;
            }
            err = SWO_SUCCESS;
        }
    }

    return err;
}
#endif  // HAVE_SEED_COOKIE

#if defined(DEBUG_OS_STACK_CONSUMPTION)
static bolos_err_t get_stack_consumption(uint8_t  mode,
                                         uint8_t *buffer_out,
                                         size_t  *buffer_out_length)
{
    bolos_err_t err    = SWO_CONDITIONS_NOT_SATISFIED;
    int         status = os_stack_operations(mode);

    *buffer_out_length = 0;
    if (status != -1) {
        U4BE_ENCODE(buffer_out, 0x00, status);
        *buffer_out_length += 4;
        err = SWO_SUCCESS;
    }

    return err;
}
#endif  // DEBUG_OS_STACK_CONSUMPTION

#if defined(HAVE_LEDGER_PKI)
static bolos_err_t pki_load_certificate(uint8_t *buffer, size_t buffer_len, uint8_t key_usage)
{
    bolos_err_t              err = SWO_CONDITIONS_NOT_SATISFIED;
    cx_ecfp_384_public_key_t public_key;

    err = os_pki_load_certificate(key_usage, buffer, buffer_len, NULL, NULL, &public_key);
    if (err == 0) {
        err = SWO_SUCCESS;
    }
    explicit_bzero(&public_key, sizeof(cx_ecfp_384_public_key_t));

    return err;
}
#endif  // HAVE_LEDGER_PKI

/* Exported functions --------------------------------------------------------*/
bolos_err_t os_io_handle_default_apdu(uint8_t                  *buffer_in,
                                      size_t                    buffer_in_length,
                                      uint8_t                  *buffer_out,
                                      size_t                   *buffer_out_length,
                                      os_io_apdu_post_action_t *post_action)
{
    bolos_err_t err = SWO_CONDITIONS_NOT_SATISFIED;

    if (!buffer_in || !buffer_in_length || !buffer_out || !buffer_out_length) {
        return *post_action;
    }
    if (post_action) {
        *post_action = OS_IO_APDU_POST_ACTION_NONE;
    }

    if (DEFAULT_APDU_CLA == buffer_in[APDU_OFF_CLA]) {
        switch (buffer_in[APDU_OFF_INS]) {
            case DEFAULT_APDU_INS_GET_VERSION:
                if (!buffer_in[APDU_OFF_P1] && !buffer_in[APDU_OFF_P2]) {
                    err = get_version(buffer_out, buffer_out_length);
                }
                else {
                    err = SWO_INVALID_CLA;
                    goto end;
                }
                break;

#if defined(HAVE_SEED_COOKIE)
            case DEFAULT_APDU_INS_GET_SEED_COOKIE:
                if (!buffer_in[APDU_OFF_P1] && !buffer_in[APDU_OFF_P2]) {
                    err = get_seed_cookie(buffer_out, buffer_out_length);
                }
                else {
                    err = SWO_INVALID_CLA;
                    goto end;
                }
                break;
#endif  // HAVE_SEED_COOKIE

#if defined(DEBUG_OS_STACK_CONSUMPTION)
            case DEFAULT_APDU_INS_STACK_CONSUMPTION:
                if (!buffer_in[APDU_OFF_P2] && !buffer_in[APDU_OFF_LC]) {
                    err = get_stack_consumption(
                        buffer_in[APDU_OFF_P1], buffer_out, buffer_out_length);
                }
                else {
                    err = SWO_INVALID_CLA;
                    goto end;
                }
                break;
#endif  // DEBUG_OS_STACK_CONSUMPTION

            case DEFAULT_APDU_INS_APP_EXIT:
                if (!buffer_in[APDU_OFF_P1] && !buffer_in[APDU_OFF_P2]) {
                    *buffer_out_length = 0;
#if !defined(HAVE_BOLOS)
                    if (post_action) {
                        *post_action = OS_IO_APDU_POST_ACTION_EXIT;
                    }
#endif  // !HAVE_BOLOS
                    err = SWO_SUCCESS;
                }
                else {
                    err = SWO_INVALID_CLA;
                    goto end;
                }
                break;

#if defined(HAVE_LEDGER_PKI)
            case DEFAULT_APDU_INS_LOAD_CERTIFICATE:
                *buffer_out_length = 0;
                err                = pki_load_certificate(
                    &buffer_in[APDU_OFF_LC + 1], buffer_in[APDU_OFF_LC], buffer_in[APDU_OFF_P1]);
                break;
#endif  // HAVE_LEDGER_PKI

            default:
                err = SWO_INVALID_CLA;
                goto end;
                break;
        }
    }

end:
    return err;
}
