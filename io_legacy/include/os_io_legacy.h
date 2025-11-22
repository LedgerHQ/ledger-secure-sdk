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

#define IO_APDU_BUFFER_SIZE OS_IO_BUFFER_SIZE + 1

#define G_io_apdu_buffer G_io_tx_buffer

// deprecated
#define G_io_apdu_media G_io_app.apdu_media
// deprecated
#define G_io_apdu_state G_io_app.apdu_state

extern unsigned char G_io_asynch_ux_callback;

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern unsigned char G_io_seproxyhal_spi_buffer[OS_IO_SEPH_BUFFER_SIZE];

/* Exported functions prototypes--------------------------------------------- */
SYSCALL void io_seph_send(const unsigned char *buffer PLENGTH(length), unsigned short length);
SYSCALL unsigned int   io_seph_is_status_sent(void);
SYSCALL unsigned short io_seph_recv(unsigned char *buffer PLENGTH(maxlength),
                                    unsigned short        maxlength,
                                    unsigned int          flags);

#define io_seproxyhal_spi_send           io_seph_send
#define io_seproxyhal_spi_is_status_sent io_seph_is_status_sent
#define io_seproxyhal_spi_recv           io_seph_recv

void io_seproxyhal_general_status(void);
void io_seproxyhal_se_reset(void);
void io_seproxyhal_disable_io(void);
void io_seproxyhal_io_heartbeat(void);
void io_seproxyhal_request_mcu_status(void);

#ifdef HAVE_BAGL
void io_seproxyhal_display_default(const bagl_element_t *element);
#endif  // HAVE_BAGL

void io_seproxyhal_play_tune(tune_index_e tune_index);

unsigned int io_seph_is_status_sent(void);

unsigned short io_exchange(unsigned char channel_and_flags, unsigned short tx_len);
void           io_seproxyhal_init(void);
void           USB_power(unsigned char enabled);
#ifdef HAVE_BLE
void BLE_power(unsigned char powered, const char *discovered_name);
#endif  // HAVE_BLE

unsigned int os_io_seph_recv_and_process(unsigned int dont_process_ux_events);

unsigned char io_event(unsigned char channel);

int io_legacy_apdu_rx(uint8_t handle_ux_events);
int io_legacy_apdu_tx(const unsigned char *buffer, unsigned short length);
