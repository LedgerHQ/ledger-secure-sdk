/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "os_math.h"
#include "decorators.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    IO_APDU_MEDIA_NONE    = 0,  // not correctly in an apdu exchange
    IO_APDU_MEDIA_USB_HID = 1,
    IO_APDU_MEDIA_BLE,
    IO_APDU_MEDIA_NFC,
    IO_APDU_MEDIA_USB_CCID,
    IO_APDU_MEDIA_USB_WEBUSB,
    IO_APDU_MEDIA_RAW,
    IO_APDU_MEDIA_U2F,
} io_apdu_media_t;

typedef enum {
    APDU_IDLE,
    APDU_BLE,
    APDU_BLE_WAIT_NOTIFY,
    APDU_NFC,
    APDU_NFC_M24SR,
    APDU_NFC_M24SR_SELECT,
    APDU_NFC_M24SR_FIRST,
    APDU_NFC_M24SR_RAPDU,
    APDU_USB_HID,
    APDU_USB_CCID,
    APDU_U2F,
    APDU_RAW,
    APDU_USB_WEBUSB,
} io_apdu_state_e;

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    io_apdu_state_e apdu_state;   // by default
    unsigned short  apdu_length;  // total length to be received
    unsigned short  io_flags;     // flags to be set when calling io_exchange
    io_apdu_media_t apdu_media;
#ifdef HAVE_BLE
    unsigned int plane_mode;
#endif  // HAVE_BLE
} io_seph_app_t;

/* Exported defines   --------------------------------------------------------*/
#ifdef IO_SEPROXYHAL_BUFFER_SIZE_B
#undef IO_SEPROXYHAL_BUFFER_SIZE_B
#endif  // IO_SEPROXYHAL_BUFFER_SIZE_B
#define IO_SEPROXYHAL_BUFFER_SIZE_B OS_IO_SEPH_BUFFER_SIZE

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern io_seph_app_t G_io_app;
#ifdef HAVE_IO_U2F
#include "u2f_service.h"
extern u2f_service_t G_io_u2f;
#endif  // HAVE_IO_U2F

/* Exported functions prototypes--------------------------------------------- */
