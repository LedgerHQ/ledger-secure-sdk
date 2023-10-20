/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "os_math.h"
#include "decorators.h"
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"

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
    unsigned int  plane_mode;
#endif  // HAVE_BLE
} io_seph_app_t;

/* Exported defines   --------------------------------------------------------*/


#define CHANNEL_APDU           0
#define CHANNEL_KEYBOARD       1
#define CHANNEL_SPI            2
#define IO_RESET_AFTER_REPLIED 0x80
#define IO_RECEIVE_DATA        0x40
#define IO_RETURN_AFTER_TX     0x20
#define IO_ASYNCH_REPLY        0x10  // avoid apdu state reset if tx_len == 0 when we're expected to reply
#define IO_FLAGS               0xF8

#define IO_CACHE 1

#define G_io_apdu_buffer G_io_rx_buffer

// deprecated
#define G_io_apdu_media G_io_app.apdu_media
// deprecated
#define G_io_apdu_state G_io_app.apdu_state

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern io_seph_app_t G_io_app;
extern unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

/* Exported functions prototypes--------------------------------------------- */
SYSCALL unsigned int   io_seph_is_status_sent(void);
SYSCALL unsigned short io_seph_recv(unsigned char *buffer PLENGTH(maxlength),
                                    unsigned short        maxlength,
                                    unsigned int          flags);

#define io_seproxyhal_spi_send           (void)
#define io_seproxyhal_spi_is_status_sent io_seph_is_status_sent
#define io_seproxyhal_spi_recv           io_seph_recv


void io_seproxyhal_general_status(void);
void io_seproxyhal_se_reset(void);
void io_seproxyhal_disable_io(void);
void io_seproxyhal_io_heartbeat(void);

#ifdef HAVE_BAGL
void io_seproxyhal_display_default(const bagl_element_t *element);
#endif  // HAVE_BAGL

#ifdef HAVE_PIEZO_SOUND
void io_seproxyhal_play_tune(tune_index_e tune_index);
#endif  // HAVE_PIEZO_SOUND

unsigned int io_seph_is_status_sent(void);

unsigned short io_exchange(unsigned char channel_and_flags, unsigned short tx_len);
void io_seproxyhal_init(void);
void USB_power(unsigned char enabled);
#ifdef HAVE_BLE
void BLE_power(unsigned char powered, const char *discovered_name);
#endif  // HAVE_BLE
