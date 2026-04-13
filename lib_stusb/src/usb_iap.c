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
#include "ledger_protocol.h"
#include "usbd_ledger.h"
#include "usb_iap.h"
#include "seproxyhal_protocol.h"

/* Private enumerations ------------------------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ledger_protocol_t protocol_data = {};

/* Exported variables --------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static void USB_LEDGER_IAP_send_packet(uint8_t *buffer, uint16_t length)
{
    if (length) {
        unsigned char hdr[3];
        hdr[0] = SEPROXYHAL_TAG_USB_IAP_SEND;
        hdr[1] = length >> 8;
        hdr[2] = length;
        os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, hdr, 3, NULL);
        os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, length, NULL);
    }
}

/* Exported functions --------------------------------------------------------*/

void USB_LEDGER_IAP_init(void)
{
    memset(&protocol_data, 0, sizeof(protocol_data));
    protocol_data.mtu = sizeof(USBD_LEDGER_protocol_chunk_buffer);

    LEDGER_PROTOCOL_init(&protocol_data, OS_IO_PACKET_TYPE_USB_IAP_APDU);
}

int USB_LEDGER_IAP_send_apdu(const uint8_t *apdu_buf, uint16_t apdu_buf_length)
{
    uint32_t status = -1;

    ledger_protocol_result_t result = LEDGER_PROTOCOL_tx(&protocol_data,
                                                         apdu_buf,
                                                         apdu_buf_length,
                                                         USBD_LEDGER_protocol_chunk_buffer,
                                                         sizeof(USBD_LEDGER_protocol_chunk_buffer),
                                                         0);

    if (result != LP_SUCCESS) {
        goto error;
    }
    if (protocol_data.tx_chunk_length >= 2) {
        USB_LEDGER_IAP_send_packet(USBD_LEDGER_protocol_chunk_buffer,
                                   protocol_data.tx_chunk_length);
    }

    while (protocol_data.tx_apdu_buffer) {
        result = LEDGER_PROTOCOL_tx(&protocol_data,
                                    NULL,
                                    0,
                                    USBD_LEDGER_protocol_chunk_buffer,
                                    sizeof(USBD_LEDGER_protocol_chunk_buffer),
                                    0);
        if (result != LP_SUCCESS) {
            goto error;
        }
        if (protocol_data.tx_chunk_length >= 2) {
            USB_LEDGER_IAP_send_packet(USBD_LEDGER_protocol_chunk_buffer,
                                       protocol_data.tx_chunk_length);
        }
    }
    status = 0;

error:
    return status;
}

int USB_LEDGER_iap_rx_seph_evt(uint8_t *seph_buffer,
                               uint16_t seph_buffer_length,
                               uint8_t *apdu_buffer,
                               uint16_t apdu_buffer_max_length)
{
    int status = -1;

    if (seph_buffer[1] != SEPROXYHAL_TAG_USB_IAP_EVENT || seph_buffer_length < 4) {
        goto error;
    }

    ledger_protocol_result_t result = LEDGER_PROTOCOL_rx(&protocol_data,
                                                         &seph_buffer[4],
                                                         seph_buffer_length - 4,
                                                         USBD_LEDGER_protocol_chunk_buffer,
                                                         sizeof(USBD_LEDGER_protocol_chunk_buffer),
                                                         apdu_buffer,
                                                         apdu_buffer_max_length,
                                                         0);
    if (result != LP_SUCCESS) {
        goto error;
    }

    if (protocol_data.tx_chunk_length > 0) {
        // Tx chunck was generated while processing rx
        // For example MTU request -> MTU response
        USB_LEDGER_IAP_send_packet(USBD_LEDGER_protocol_chunk_buffer,
                                   protocol_data.tx_chunk_length);
        protocol_data.tx_chunk_length = 0;
    }

    if (protocol_data.rx_apdu_status == APDU_STATUS_COMPLETE) {
        // Should not happen as it is verified by LEDGER_PROTOCOL_rx() already
        if (apdu_buffer_max_length < protocol_data.rx_apdu_length) {
            status = -1;
        }
        else {
            status = protocol_data.rx_apdu_length;
        }
        protocol_data.rx_apdu_status = APDU_STATUS_WAITING;
    }

error:
    return status;
}
