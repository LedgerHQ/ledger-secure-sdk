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

#ifdef HAVE_CCID_USB
/* Includes ------------------------------------------------------------------*/
#include "usbd_ioreq.h"
#include "usbd_ledger.h"
#include "usbd_ledger_ccid.h"
#include "ccid_transport.h"
#include "ccid_cmd.h"
#include "ccid_types.h"

#pragma GCC diagnostic ignored "-Wcast-qual"

#ifdef HAVE_PRINTF
// #define LOG_IO PRINTF
#define LOG_IO(...)
#else  // !HAVE_PRINTF
#define LOG_IO(...)
#endif  // !HAVE_PRINTF

/* Private enumerations ------------------------------------------------------*/
enum ledger_ccid_state_e {
    LEDGER_CCID_STATE_IDLE,
    LEDGER_CCID_STATE_BUSY,
};

/* Private defines------------------------------------------------------------*/
#define LEDGER_CCID_BULK_EPIN_ADDR  (0x83)
#define LEDGER_CCID_BULK_EPIN_SIZE  (0x40)
#define LEDGER_CCID_BULK_EPOUT_ADDR (0x03)
#define LEDGER_CCID_BULK_EPOUT_SIZE (0x40)

#define CCID_FUNCTIONAL_DESCRIPTOR_TYPE (0x21)

#define LEDGER_CCID_INTERRUPT_EPIN_ADDR (0x84)
#define LEDGER_CCID_INTERRUPT_EPIN_SIZE (0x10)

// CCID Class-Specific Requests
#define REQ_ABORT                 (0x01)
#define REQ_GET_CLOCK_FREQUENCIES (0x02)
#define REQ_GET_DATA_RATES        (0x03)

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    uint8_t alt_setting;
    uint8_t state;  // ledger_ccid_state_e

    // Transport
    ccid_device_t device;
} ledger_ccid_handle_t;

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ledger_ccid_handle_t ledger_ccid_handle;
static uint8_t              ledger_ccid_transport_packet_buffer[LEDGER_CCID_BULK_EPIN_SIZE];

/* Exported variables --------------------------------------------------------*/
const usbd_end_point_info_t LEDGER_CCID_Bulk_end_point_info = {
    .ep_in_addr  = LEDGER_CCID_BULK_EPIN_ADDR,
    .ep_in_size  = LEDGER_CCID_BULK_EPIN_SIZE,
    .ep_out_addr = LEDGER_CCID_BULK_EPOUT_ADDR,
    .ep_out_size = LEDGER_CCID_BULK_EPOUT_SIZE,
    .ep_type     = USBD_EP_TYPE_BULK,
};

// clang-format off
const uint8_t LEDGER_CCID_Bulk_descriptor[77] = {
    /************** Interface descriptor ******************************/
    USB_LEN_IF_DESC,                       // bLength
    USB_DESC_TYPE_INTERFACE,               // bDescriptorType    : interface
    0x00,                                  // bInterfaceNumber   : 0 (dynamic)
    0x00,                                  // bAlternateSetting  : 0
    0x02,                                  // bNumEndpoints      : 2
    0x0B,                                  // bInterfaceClass    : Smart Card Device Class
    0x00,                                  // bInterfaceSubClass : none
    0x00,                                  // bInterfaceProtocol : none
    USBD_IDX_INTERFACE_STR,                // iInterface         :

    /************** Functional Descriptor *****************************/
    0x36,                                  // bLength
    CCID_FUNCTIONAL_DESCRIPTOR_TYPE,       // bDescriptorType        : Functional
    0x10, 0x01,                            // bcdCCID                : V1.10
    0x00,                                  // bMaxSlotIndex          : 0
    0x03,                                  // bVoltageSupport        : 3V or 5V
    0x01, 0x00, 0x00, 0x00,                // dwProtocols            : Protocol T=0
    0x10, 0x0E, 0x00, 0x00,                // dwDefaultClock         : 3.6Mhz
    0x10, 0x0E, 0x00, 0x00,                // dwMaximumClock         : 3.6Mhz
    0x00,                                  // bNumClockSupported     : default and maximum clock
    0xCD, 0x25, 0x00, 0x00,                // dwDataRate             : 9677 bps
    0xCD, 0x25, 0x00, 0x00,                // dwMaxDataRate          : 9677 bps
    0x00,                                  // bNumDataRatesSupported : between default & max data rate
    0x00, 0x00, 0x00, 0x00,                // dwMaxIFSD              : max IFSD for protocol T=1
    0x00, 0x00, 0x00, 0x00,                // dwSynchProtocols       : None supported
    0x00, 0x00, 0x00, 0x00,                // dwMechanical           : No special characteristics
    0xBA, 0x06, 0x02, 0x00,                // dwFeatures             : - Automatic parameter configuration based on ATR data
                                           //                          - Automatic ICC voltage selection
                                           //                          - Automatic ICC clock frequency change according to active parameters provided by the Host or self determined
                                           //                          - Automatic baud rate change according to active parameters provided by the Host or self determined
                                           //                          - Automatic PPS made by the CCID according to the active parameters
                                           //                          - NAD value other than 00 accepted (T=1 protocol in use)
                                           //                          - Automatic IFSD exchange as first exchange (T=1 protocol in use)
                                           //                          - Short APDU level exchange with CCID
    0x0F, 0x01, 0x00, 0x00,                // dwMaxCCIDMessageLength : 261 + 10
    0x00,                                  // bClassGetResponse      : default class value is 0x00
    0x00,                                  // bClassEnvelope         : default class value is 0x00
    0x00, 0x00,                            // wLcdLayout             : No LCD
    0x03,                                  // bPINSupport            : Pin verification & modification supported
    0x01,                                  // bMaxCCIDBusySlots      : Max is 1

    /************** Endpoint descriptor *******************************/
    USB_LEN_EP_DESC,                       // bLength
    USB_DESC_TYPE_ENDPOINT,                // bDescriptorType
    LEDGER_CCID_BULK_EPIN_ADDR,            // bEndpointAddress
    USBD_EP_TYPE_BULK,                     // bmAttributes
    LEDGER_CCID_BULK_EPIN_SIZE,            // wMaxPacketSize
    0x00,                                  // wMaxPacketSize
    0x00,                                  // Ignored

    /************** Endpoint descriptor *******************************/
    USB_LEN_EP_DESC,                       // bLength
    USB_DESC_TYPE_ENDPOINT,                // bDescriptorType
    LEDGER_CCID_BULK_EPOUT_ADDR,           // bEndpointAddress
    USBD_EP_TYPE_BULK,                     // bmAttributes
    LEDGER_CCID_BULK_EPOUT_SIZE,           // wMaxPacketSize
    0x00,                                  // wMaxPacketSize
    0x00,                                  // Ignored
};
// clang-format on

const usbd_class_info_t USBD_LEDGER_CCID_Bulk_class_info = {
    .type = USBD_LEDGER_CLASS_CCID_BULK,

    .end_point = &LEDGER_CCID_Bulk_end_point_info,

    .init     = USBD_LEDGER_CCID_init,
    .de_init  = USBD_LEDGER_CCID_de_init,
    .setup    = USBD_LEDGER_CCID_setup,
    .data_in  = USBD_LEDGER_CCID_data_in,
    .data_out = USBD_LEDGER_CCID_data_out,

    .send_packet = USBD_LEDGER_CCID_send_packet,
    .is_busy     = USBD_LEDGER_CCID_is_busy,

    .data_ready = USBD_LEDGER_CCID_data_ready,

    .interface_descriptor      = LEDGER_CCID_Bulk_descriptor,
    .interface_descriptor_size = sizeof(LEDGER_CCID_Bulk_descriptor),

    .interface_association_descriptor      = NULL,
    .interface_association_descriptor_size = 0,

    .bos_descriptor      = NULL,
    .bos_descriptor_size = 0,

    .cookie = &ledger_ccid_handle,
};

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
uint8_t USBD_LEDGER_CCID_init(USBD_HandleTypeDef *pdev, void *cookie)
{
    if (!cookie) {
        return USBD_FAIL;
    }

    UNUSED(pdev);

    ledger_ccid_handle_t *handle = (ledger_ccid_handle_t *) PIC(cookie);

    memset(handle, 0, sizeof(ledger_ccid_handle_t));

    handle->state                                  = LEDGER_CCID_STATE_IDLE;
    handle->device.card_inserted                   = 1;
    handle->device.slot_status                     = CCID_SLOT_STATUS_IDLE;
    handle->device.transport.rx_msg_buffer         = USBD_LEDGER_io_buffer;
    handle->device.transport.rx_msg_buffer_size    = sizeof(USBD_LEDGER_io_buffer);
    handle->device.transport.tx_packet_buffer      = ledger_ccid_transport_packet_buffer;
    handle->device.transport.tx_packet_buffer_size = sizeof(ledger_ccid_transport_packet_buffer);

    CCID_TRANSPORT_init(&handle->device.transport);

    USBD_LL_PrepareReceive(pdev, LEDGER_CCID_BULK_EPOUT_ADDR, NULL, LEDGER_CCID_BULK_EPOUT_SIZE);

    return USBD_OK;
}

uint8_t USBD_LEDGER_CCID_de_init(USBD_HandleTypeDef *pdev, void *cookie)
{
    UNUSED(pdev);
    UNUSED(cookie);

    return USBD_OK;
}

uint8_t USBD_LEDGER_CCID_setup(USBD_HandleTypeDef *pdev, void *cookie, USBD_SetupReqTypedef *req)
{
    if (!pdev || !cookie || !req) {
        return USBD_FAIL;
    }

    uint8_t               ret    = USBD_OK;
    ledger_ccid_handle_t *handle = (ledger_ccid_handle_t *) PIC(cookie);

    uint8_t request = req->bmRequest & (USB_REQ_TYPE_MASK | USB_REQ_RECIPIENT_MASK);

    if (request == (USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_INTERFACE)) {
        switch (req->bRequest) {
            case USB_REQ_GET_INTERFACE:
                if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                    USBD_CtlSendData(pdev, &handle->alt_setting, 1);
                }
                else {
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                }
                break;

            case USB_REQ_SET_INTERFACE:
                if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                    handle->alt_setting = (uint8_t) (req->wValue);
                }
                else {
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                }
                break;

            case USB_REQ_CLEAR_FEATURE:
                break;

            default:
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
                break;
        }
    }
    else if (request == (USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_INTERFACE)) {
        switch (req->bRequest) {
            case REQ_ABORT:
                if ((req->bmRequest & 0x80U) == 0U) {
                    uint8_t bSlot = (uint8_t) req->wValue;
                    uint8_t bSeq  = (uint8_t) (req->wValue >> 8);
                    if (CCID_CMD_abort(&handle->device, bSlot, bSeq)) {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                }
                else {
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                }
                break;

            case REQ_GET_CLOCK_FREQUENCIES:
                // Not supported : bNumClockSupported == 0
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
                break;

            case REQ_GET_DATA_RATES:
                // Not supported : bNumClockSupported == 0
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
                break;

            default:
                USBD_CtlError(pdev, req);
                ret = USBD_FAIL;
                break;
        }
    }
    else {
        ret = USBD_FAIL;
    }

    return ret;
}

uint8_t USBD_LEDGER_CCID_ep0_rx_ready(USBD_HandleTypeDef *pdev, void *cookie)
{
    UNUSED(pdev);
    UNUSED(cookie);

    return USBD_OK;
}

uint8_t USBD_LEDGER_CCID_data_in(USBD_HandleTypeDef *pdev, void *cookie, uint8_t ep_num)
{
    if (!cookie) {
        return USBD_FAIL;
    }

    UNUSED(pdev);
    UNUSED(ep_num);

    ledger_ccid_handle_t *handle = (ledger_ccid_handle_t *) PIC(cookie);
    if (handle->device.transport.tx_message_buffer) {
        CCID_TRANSPORT_tx(&handle->device.transport, NULL, 0);
        if (handle->device.transport.tx_packet_length) {
            handle->state = LEDGER_CCID_STATE_BUSY;
            USBD_LL_Transmit(pdev,
                             LEDGER_CCID_BULK_EPIN_ADDR,
                             handle->device.transport.tx_packet_buffer,
                             handle->device.transport.tx_packet_length,
                             0);
        }
    }
    if (!handle->device.transport.tx_message_buffer) {
        handle->device.transport.tx_packet_length = 0;
        handle->state                             = LEDGER_CCID_STATE_IDLE;
    }

    return USBD_OK;
}

uint8_t USBD_LEDGER_CCID_data_out(USBD_HandleTypeDef *pdev,
                                  void               *cookie,
                                  uint8_t             ep_num,
                                  uint8_t            *packet,
                                  uint16_t            packet_length)
{
    if (!pdev || !cookie || !packet) {
        return USBD_FAIL;
    }

    UNUSED(ep_num);

    ledger_ccid_handle_t *handle = (ledger_ccid_handle_t *) PIC(cookie);

    CCID_TRANSPORT_rx(&handle->device.transport, packet, packet_length);

    USBD_LL_PrepareReceive(pdev, LEDGER_CCID_BULK_EPOUT_ADDR, NULL, LEDGER_CCID_BULK_EPOUT_SIZE);

    return USBD_OK;
}

uint8_t USBD_LEDGER_CCID_send_packet(USBD_HandleTypeDef *pdev,
                                     void               *cookie,
                                     uint8_t             packet_type,
                                     const uint8_t      *packet,
                                     uint16_t            packet_length,
                                     uint32_t            timeout_ms)
{
    if (!pdev || !cookie || !packet) {
        return USBD_FAIL;
    }

    UNUSED(packet_type);

    uint8_t               ret    = USBD_OK;
    ledger_ccid_handle_t *handle = (ledger_ccid_handle_t *) PIC(cookie);

    CCID_TRANSPORT_tx(&handle->device.transport, packet, packet_length);

    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        if (handle->state == LEDGER_CCID_STATE_IDLE) {
            if (handle->device.transport.tx_packet_length) {
                handle->state = LEDGER_CCID_STATE_BUSY;
                ret           = USBD_LL_Transmit(pdev,
                                       LEDGER_CCID_BULK_EPIN_ADDR,
                                       handle->device.transport.tx_packet_buffer,
                                       handle->device.transport.tx_packet_length,
                                       timeout_ms);
            }
            else {
                ret = USBD_FAIL;
            }
        }
        else {
            ret = USBD_BUSY;
        }
    }
    else {
        ret = USBD_FAIL;
    }

    return ret;
}

bool USBD_LEDGER_CCID_is_busy(void *cookie)
{
    ledger_ccid_handle_t *handle = (ledger_ccid_handle_t *) PIC(cookie);
    bool busy = false;

    if (handle->state == LEDGER_CCID_STATE_BUSY) {
        busy = true;
    }

    return busy;
}

int32_t USBD_LEDGER_CCID_data_ready(USBD_HandleTypeDef *pdev,
                                    void               *cookie,
                                    uint8_t            *buffer,
                                    uint16_t            max_length)
{
    int32_t status = 0;

    UNUSED(pdev);

    if (!cookie || !buffer) {
        return -1;
    }
    ledger_ccid_handle_t *handle = (ledger_ccid_handle_t *) PIC(cookie);

    if (handle->device.transport.rx_msg_status != CCID_MSG_STATUS_COMPLETE) {
        return status;
    }

    // Check slot
    if (handle->device.transport.bulk_msg_header.out.slot_number > 1) {
        // Bad slot
        handle->device.transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_NOT_PRESENT | CCID_SLOT_STATUS_CMD_FAILED;
        handle->device.transport.bulk_msg_header.in.error = CCID_ERROR_CMD_SLOT_DOES_NOT_EXIST;
    }
    else if ((handle->device.card_inserted == 0)
             && (handle->device.transport.bulk_msg_header.out.msg_type
                 != CCID_COMMAND_PC_TO_RDR_ICC_POWER_OFF)
             && (handle->device.transport.bulk_msg_header.out.msg_type
                 != CCID_COMMAND_PC_TO_RDR_ABORT)) {
        // Card not inserted
        handle->device.transport.bulk_msg_header.in.status
            = CCID_SLOT_STATUS_ICC_NOT_PRESENT | CCID_SLOT_STATUS_CMD_FAILED;
        handle->device.transport.bulk_msg_header.in.error = CCID_ERROR_ICC_MUTE;
    }

    status = CCID_CMD_process(&handle->device);
    if (status == 0) {
        // Send response
        USBD_LEDGER_CCID_send_packet(pdev,
                                     cookie,
                                     OS_IO_PACKET_TYPE_USB_CCID_APDU,
                                     handle->device.transport.rx_msg_buffer,
                                     handle->device.transport.bulk_msg_header.in.length,
                                     0);
    }
    else if (status == 1) {
        // Transfer to upper layer
        if (handle->device.transport.rx_msg_length + 1 > max_length) {
            status = -1;
        } else {
            buffer[0] = OS_IO_PACKET_TYPE_USB_CCID_APDU;
            memmove(&buffer[1],
                    handle->device.transport.rx_msg_buffer,
                    handle->device.transport.rx_msg_length);
            status = handle->device.transport.rx_msg_length + 1;
        }
    }
    handle->device.transport.rx_msg_status = CCID_MSG_STATUS_WAITING;

    return status;
}

#endif  // HAVE_CCID_USB
