/* @BANNER@ */

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
#include "os_seed.h"
#include "os_app.h"
#include "os_registry.h"
#include "os_io_default_apdu.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void get_version(uint8_t *buffer_out, size_t *buffer_out_length);

#if defined(HAVE_SEED_COOKIE)
static void get_seed_cookie(uint8_t *buffer_out, size_t *buffer_out_length);
#endif  // HAVE_SEED_COOKIE
#if defined(DEBUG_OS_STACK_CONSUMPTION)
static void get_stack_consumption(uint8_t mode, uint8_t *buffer_out, size_t *buffer_out_length);
#endif  // DEBUG_OS_STACK_CONSUMPTION

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static void get_version(uint8_t *buffer_out, size_t *buffer_out_length)
{
    size_t max_buffer_out_length = *buffer_out_length;
    int    str_length            = 0;

    *buffer_out_length                 = 0;
    buffer_out[(*buffer_out_length)++] = 1;  // format ID

#if defined(HAVE_BOLOS)
    if (max_buffer_out_length >= (strlen("BOLOS") + strlen(VERSION) + 3 + 2)) {
        buffer_out[(*buffer_out_length)++] = 1;  // format ID
        str_length                         = strlen("BOLOS");
        buffer_out[(*buffer_out_length)++] = str_length;
        strcpy((char *) (&buffer_out[*buffer_out_length]), "BOLOS");
        *buffer_out_length += str_length;
        str_length                         = strlen(VERSION);
        buffer_out[(*buffer_out_length)++] = str_length;
        strcpy((char *) (&buffer_out[*buffer_out_length]), VERSION);
        *buffer_out_length += str_length;
        U2BE_ENCODE(buffer_out, *buffer_out_length, SWO_SUCCESS);
        *buffer_out_length += 2;
    }
#else   // !HAVE_BOLOS
    if (max_buffer_out_length >= 3) {
        buffer_out[(*buffer_out_length)++] = 1;  // format ID
        str_length
            = os_registry_get_current_app_tag(BOLOS_TAG_APPNAME,
                                              &buffer_out[(*buffer_out_length) + 1],
                                              max_buffer_out_length - *buffer_out_length - 3);
        buffer_out[(*buffer_out_length)++] = str_length;
        *buffer_out_length += str_length;
        str_length
            = os_registry_get_current_app_tag(BOLOS_TAG_APPVERSION,
                                              &buffer_out[(*buffer_out_length) + 1],
                                              max_buffer_out_length - *buffer_out_length - 3);
        buffer_out[(*buffer_out_length)++] = str_length;
        *buffer_out_length += str_length;
        U2BE_ENCODE(buffer_out, *buffer_out_length, SWO_SUCCESS);
        *buffer_out_length += 2;
    }
#endif  // !HAVE_BOLOS
}

#if defined(HAVE_SEED_COOKIE)
static void get_seed_cookie(uint8_t *buffer_out, size_t *buffer_out_length)
{
    size_t max_buffer_out_length = *buffer_out_length;

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
            U2BE_ENCODE(buffer_out, *buffer_out_length, SWO_SUCCESS);
            *buffer_out_length += 2;
        }
    }
    else {
        U2BE_ENCODE(buffer_out, *buffer_out_length, 0x6985);
        *buffer_out_length += 2;
    }
}
#endif  // HAVE_SEED_COOKIE

#if defined(DEBUG_OS_STACK_CONSUMPTION)
static void get_stack_consumption(uint8_t mode, uint8_t *buffer_out, size_t *buffer_out_length)
{
    int status = os_stack_operations(mode);

    *buffer_out_length = 0;
    if (status != -1) {
        U4BE_ENCODE(buffer_out, 0x00, status);
        *buffer_out_length += 4;
        U2BE_ENCODE(buffer_out, *buffer_out_length, SWO_SUCCESS);
        *buffer_out_length += 2;
    }
}
#endif  // DEBUG_OS_STACK_CONSUMPTION

/* Exported functions --------------------------------------------------------*/
os_io_apdu_post_action_t os_io_handle_default_apdu(uint8_t *buffer_in,
                                                   size_t   buffer_in_length,
                                                   uint8_t *buffer_out,
                                                   size_t  *buffer_out_length)
{
    os_io_apdu_post_action_t post_action = OS_IO_APDU_POST_ACTION_NONE;

    if (!buffer_in || !buffer_in_length || !buffer_out || !buffer_out_length) {
        return post_action;
    }

    if (DEFAULT_APDU_CLA == buffer_in[APDU_OFF_CLA]) {
        switch (buffer_in[APDU_OFF_INS]) {
            case DEFAULT_APDU_INS_GET_VERSION:
                if (!buffer_in[APDU_OFF_P1] && !buffer_in[APDU_OFF_P2]) {
                    get_version(buffer_out, buffer_out_length);
                }
                else {
                    U2BE_ENCODE(buffer_out, 0, 0x6E00);
                    *buffer_out_length = 2;
                }
                break;

#if defined(HAVE_SEED_COOKIE)
            case DEFAULT_APDU_INS_GET_SEED_COOKIE:
                if (!buffer_in[APDU_OFF_P1] && !buffer_in[APDU_OFF_P2]) {
                    get_seed_cookie(buffer_out, buffer_out_length);
                }
                else {
                    U2BE_ENCODE(buffer_out, 0, 0x6E00);
                    *buffer_out_length = 2;
                }
                break;
#endif  // HAVE_SEED_COOKIE

#if defined(DEBUG_OS_STACK_CONSUMPTION)
            case DEFAULT_APDU_INS_STACK_CONSUMPTION:
                if (!buffer_in[APDU_OFF_P2] && !buffer_in[APDU_OFF_LC]) {
                    get_stack_consumption(buffer_in[APDU_OFF_P1], buffer_out, buffer_out_length);
                }
                else {
                    U2BE_ENCODE(buffer_out, 0, 0x6E00);
                    *buffer_out_length = 2;
                }
                break;
#endif  // DEBUG_OS_STACK_CONSUMPTION

            case DEFAULT_APDU_INS_APP_EXIT:
                if (!buffer_in[APDU_OFF_P1] && !buffer_in[APDU_OFF_P2]) {
                    U2BE_ENCODE(buffer_out, 0, SWO_SUCCESS);
                    *buffer_out_length = 2;
#if !defined(HAVE_BOLOS)
                    post_action = OS_IO_APDU_POST_ACTION_EXIT;
#endif  // !HAVE_BOLOS
                }
                else {
                    U2BE_ENCODE(buffer_out, 0, 0x6E00);
                    *buffer_out_length = 2;
                }
                break;

            default:
                U2BE_ENCODE(buffer_out, 0, 0x6E01);
                *buffer_out_length = 2;
                break;
        }
    }

    return post_action;
}
