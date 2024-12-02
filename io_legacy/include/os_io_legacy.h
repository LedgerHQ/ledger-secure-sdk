/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "os_math.h"
#include "decorators.h"
#include "os_io_legacy_types.h"
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

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
extern unsigned char G_io_seproxyhal_spi_buffer[OS_IO_SEPH_BUFFER_SIZE];

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
void           io_seproxyhal_init(void);
void           USB_power(unsigned char enabled);
#ifdef HAVE_BLE
void BLE_power(unsigned char powered, const char *discovered_name);
#endif  // HAVE_BLE

unsigned char io_event(unsigned char channel);

int io_legacy_apdu_rx(void);
int io_legacy_apdu_tx(const unsigned char *buffer, unsigned short length);
