/*****************************************************************************
 *   (c) 2021 Ledger SAS.
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

#include <stdint.h>
#include <string.h>

#include "os.h"
#include "os_io_legacy_types.h"
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"
#include "os_io_default_apdu.h"
#include "seproxyhal_protocol.h"
#include "write.h"
#include "offsets.h"
#include "io.h"

#ifdef HAVE_IO_USB
#include "usbd_ledger.h"
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
#include "ble_ledger.h"
#endif  // HAVE_BLE

#ifdef HAVE_NFC_READER
#include "nfc_ledger.h"
#endif  // HAVE_NFC_READER

#ifdef HAVE_SWAP
#include "swap.h"
#endif

#include "status_words.h"

static uint8_t need_to_start_io;

uint8_t G_io_seproxyhal_spi_buffer[OS_IO_SEPH_BUFFER_SIZE];

#ifdef HAVE_BAGL
WEAK void io_seproxyhal_display(const bagl_element_t *element)
{
    io_seph_ux_display_bagl_element(element);
}
#endif  // HAVE_BAGL

// This function can be used to declare a callback to SEPROXYHAL_TAG_TICKER_EVENT in the application
WEAK void app_ticker_event_callback(void) {}

WEAK unsigned char io_event(unsigned char channel)
{
    UNUSED(channel);
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;
#ifdef HAVE_SE_TOUCH
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;
#endif  // HAVE_SE_TOUCH
        case SEPROXYHAL_TAG_TICKER_EVENT:
            app_ticker_event_callback();
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
            break;
        default:
            UX_DEFAULT_EVENT();
            break;
    }

    return 1;
}

WEAK void io_init()
{
    need_to_start_io = 1;
}

WEAK int io_recv_command()
{
    int status = 0;

    if (need_to_start_io) {
#ifndef USE_OS_IO_STACK
        io_seproxyhal_io_heartbeat();
        io_seproxyhal_io_heartbeat();
        io_seproxyhal_io_heartbeat();
#endif  // USE_OS_IO_STACK
        os_io_start();
        need_to_start_io = 0;
    }

#ifdef FUZZING
    for (uint8_t retries = 5; retries && status <= 0; retries--) {
#else
    while (status <= 0) {
#endif
        status = io_legacy_apdu_rx(1);
    }

    return status;
}

WEAK int io_send_response_buffers(const buffer_t *rdatalist, size_t count, uint16_t sw)
{
    int    status = 0;
    size_t length = 0;

    if (rdatalist && count > 0) {
        for (size_t i = 0; i < count; i++) {
            const buffer_t *rdata = &rdatalist[i];

            if (!buffer_copy(rdata, G_io_tx_buffer + length, sizeof(G_io_tx_buffer) - length - 2)) {
                return io_send_sw(SWO_INSUFFICIENT_MEMORY);
            }
            length += rdata->size - rdata->offset;
            if (count > 1) {
                PRINTF("<= FRAG (%u/%u) RData=%.*H\n", i + 1, count, rdata->size, rdata->ptr);
            }
        }
        PRINTF("<= SW=%04X | RData=%.*H\n", sw, length, G_io_tx_buffer);
    }
    else {
        PRINTF("<= SW=%04X | RData=\n", sw);
    }

    write_u16_be(G_io_tx_buffer, length, sw);
    length += 2;

#ifdef HAVE_SWAP
    // If we are in swap mode and have validated a TX, we send it and immediately quit
    if (G_called_from_swap && G_swap_response_ready) {
        PRINTF("Swap answer is processed. Send it\n");

        if (io_legacy_apdu_tx(G_io_tx_buffer, length) >= 0) {
            PRINTF("Returning to Exchange with status %d\n", (sw == SWO_SUCCESS));
            *G_swap_signing_return_value_address = (sw == SWO_SUCCESS);
            PRINTF("os_lib_end\n");
            os_lib_end();
        }
        else {
            PRINTF("Unrecoverable\n");
#ifndef USE_OS_IO_STACK
            os_io_stop();
#endif  // USE_OS_IO_STACK
            os_sched_exit(-1);
        }
    }
#endif  // HAVE_SWAP

    status = io_legacy_apdu_tx(G_io_tx_buffer, length);

    if (status < 0) {
        status = -1;
    }

    return status;
}

#ifdef STANDARD_APP_SYNC_RAPDU
WEAK bool io_recv_and_process_event(void)
{
    int status = io_legacy_apdu_rx(1);
    if (status > 0) {
        return true;
    }

    return false;
}
#endif
