/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
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
    // cached here to avoid unavailable zone deref within IO task
    unsigned char ble_ready;
    unsigned char name_changed;
    unsigned char enabling_advertising;
    unsigned char disabling_advertising;
#endif  // HAVE_BLE

    unsigned char transfer_mode;
} io_seph_app_t;

/* Exported defines   --------------------------------------------------------*/
#ifdef HAVE_IO_U2F
#define IMPL_IO_APDU_BUFFER_SIZE (3 + 32 + 32 + 15 + 255)
#else  // !HAVE_IO_U2F
#define IMPL_IO_APDU_BUFFER_SIZE (5 + 255)
#endif  // !HAVE_IO_U2F

#ifdef CUSTOM_IO_APDU_BUFFER_SIZE
#define IO_APDU_BUFFER_SIZE MAX(IMPL_IO_APDU_BUFFER_SIZE, CUSTOM_IO_APDU_BUFFER_SIZE)
#else  // !CUSTOM_IO_APDU_BUFFER_SIZE
#define IO_APDU_BUFFER_SIZE IMPL_IO_APDU_BUFFER_SIZE
#endif  // !CUSTOM_IO_APDU_BUFFER_SIZE

#define CHANNEL_APDU           0
#define CHANNEL_KEYBOARD       1
#define CHANNEL_SPI            2
#define IO_RESET_AFTER_REPLIED 0x80
#define IO_RECEIVE_DATA        0x40
#define IO_RETURN_AFTER_TX     0x20
#define IO_ASYNCH_REPLY        0x10  // avoid apdu state reset if tx_len == 0 when we're expected to reply
#define IO_FLAGS               0xF8

#define IO_CACHE 1

/* Exported macros------------------------------------------------------------*/
#define G_io_apdu_buffer G_io_apdu_rx_buffer

/* Exported variables --------------------------------------------------------*/
extern io_seph_app_t G_io_app;

/* Exported functions prototypes--------------------------------------------- */
SYSCALL void io_seph_send(const unsigned char *buffer PLENGTH(length), unsigned short length);
SYSCALL unsigned int   io_seph_is_status_sent(void);
SYSCALL unsigned short io_seph_recv(unsigned char *buffer PLENGTH(maxlength),
                                    unsigned short        maxlength,
                                    unsigned int          flags);

#define io_seproxyhal_spi_send           io_seph_send
#define io_seproxyhal_spi_is_status_sent io_seph_is_status_sent
#define io_seproxyhal_spi_recv           io_seph_recv