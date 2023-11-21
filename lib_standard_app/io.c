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
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"
#include "os_io_default_apdu.h"
#include "seproxyhal_protocol.h"
#include "write.h"
#include "offsets.h"
#include "io.h"

#ifdef HAVE_SWAP
#include "swap.h"
#endif  // HAVE_SWAP

// TODO: Temporary workaround, at some point all status words should be defined by the SDK and
// removed from the application
#define SW_OK                    0x9000
#define SW_WRONG_RESPONSE_LENGTH 0xB000

/**
 * Variable containing the type of the APDU response to send back.
 */
static os_io_packet_type_t G_rx_packet_type;

#ifdef HAVE_BAGL
WEAK void io_seproxyhal_display(const bagl_element_t *element)
{
    io_seph_ux_display_bagl_element(element);
}
#endif  // HAVE_BAGL

// This function can be used to declare a callback to SEPROXYHAL_TAG_TICKER_EVENT in the application
WEAK void app_ticker_event_callback(void) {}

WEAK uint8_t io_event(uint8_t *buffer_in, size_t buffer_in_length)
{
    switch (buffer_in[0]) {
        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
#ifdef HAVE_BAGL
            UX_BUTTON_PUSH_EVENT(buffer_in);
#endif  // HAVE_BAGL
            break;
        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
#ifdef HAVE_BAGL
            UX_DISPLAYED_EVENT({});
#endif  // HAVE_BAGL
#ifdef HAVE_NBGL
            UX_DEFAULT_EVENT();
#endif  // HAVE_NBGL
            break;
#ifdef HAVE_NBGL
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(buffer_in);
            break;
#endif  // HAVE_NBGL
        case SEPROXYHAL_TAG_TICKER_EVENT:
            app_ticker_event_callback();
            UX_TICKER_EVENT(buffer_in, {});
            break;
        case SEPROXYHAL_TAG_ITC_EVENT:
            io_process_itc_ux_event(buffer_in, buffer_in_length);
            break;

        default:
            UX_DEFAULT_EVENT();
            break;
    }

    if (G_io_rx_buffer[0] == OS_IO_PACKET_TYPE_SEPH) {
        os_io_seph_cmd_general_status();
    }

    return 1;
}

WEAK void io_init() {}

WEAK int io_recv_command()
{
    int status = 0;

    while (!status) {
        status = os_io_rx_evt(G_io_rx_buffer, sizeof(G_io_rx_buffer), NULL);

        if (status > 0) {
            G_rx_packet_type = G_io_rx_buffer[0];
            switch (G_rx_packet_type) {
                case OS_IO_PACKET_TYPE_SE_EVT:
                case OS_IO_PACKET_TYPE_SEPH:
                    io_event(&G_io_rx_buffer[1], status - 1);
                    status = 0;
                    break;

                case OS_IO_PACKET_TYPE_RAW_APDU:
                case OS_IO_PACKET_TYPE_USB_HID_APDU:
                case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
                case OS_IO_PACKET_TYPE_BLE_APDU:
                    if (G_io_rx_buffer[OFFSET_CLA + 1] == DEFAULT_APDU_CLA) {
                        size_t buffer_out_length = sizeof(G_io_tx_buffer);
                        os_io_handle_default_apdu(
                            &G_io_rx_buffer[1], status - 1, G_io_tx_buffer, &buffer_out_length);
                        os_io_tx_cmd(G_rx_packet_type, G_io_tx_buffer, buffer_out_length, 0);
                        status = 0;
                    }
                    break;

                default:
                    status = 0;
                    break;
            }
        }
        else if (status < 0) {
            status = -1;
        }
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
                return io_send_sw(SW_WRONG_RESPONSE_LENGTH);
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
        if (os_io_tx_cmd(G_rx_packet_type, G_io_tx_buffer, length, 0) >= 0) {
            swap_finalize_exchange_sign_transaction(sw == SW_OK);
        }
        else {
            PRINTF("Unrecoverable\n");
            os_sched_exit(-1);
        }
    }
#endif  // HAVE_SWAP

    status = os_io_tx_cmd(G_rx_packet_type, G_io_tx_buffer, length, 0);

    if (status < 0) {
        status = -1;
    }

    return status;
}
