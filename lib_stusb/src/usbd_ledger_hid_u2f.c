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

#ifdef HAVE_IO_U2F
/* Includes ------------------------------------------------------------------*/
#include "lcx_crc.h"
#include "u2f_transport.h"
#include "usbd_ioreq.h"
#include "usbd_ledger.h"
#include "u2f_types.h"
#include "usbd_ledger_hid_u2f.h"

#ifdef HAVE_BOLOS
#include "cx_crc_internal.h"
#endif  // !HAVE_BOLOS

/* Private enumerations ------------------------------------------------------*/
enum ledger_hid_u2f_state_t {
    LEDGER_HID_U2F_STATE_IDLE,
    LEDGER_HID_U2F_STATE_BUSY,
};

enum ledger_hid_u2f_user_presence_t {
    LEDGER_HID_U2F_USER_PRESENCE_IDLE,
    LEDGER_HID_U2F_USER_PRESENCE_ASKING,
    LEDGER_HID_U2F_USER_PRESENCE_CONFIRMED,
};

/* Private defines------------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/
#define LEDGER_HID_U2F_EPIN_ADDR  (0x81)
#define LEDGER_HID_U2F_EPIN_SIZE  (0x40)
#define LEDGER_HID_U2F_EPOUT_ADDR (0x01)
#define LEDGER_HID_U2F_EPOUT_SIZE (LEDGER_USBD_DEFAULT_EPOUT_SIZE)

#define HID_DESCRIPTOR_TYPE        (0x21)
#define HID_REPORT_DESCRIPTOR_TYPE (0x22)

// HID Class-Specific Requests
#define REQ_SET_REPORT   (0x09)
#define REQ_GET_REPORT   (0x01)
#define REQ_SET_IDLE     (0x0A)
#define REQ_GET_IDLE     (0x02)
#define REQ_SET_PROTOCOL (0x0B)
#define REQ_GET_PROTOCOL (0x03)

#define APDU_MIN_HEADER (4 + 3)

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    // HID
    uint8_t protocol;
    uint8_t alt_setting;
    uint8_t idle_state;
    uint8_t state;  // ledger_hid_u2f_state_t

    // Transport
    u2f_transport_t transport_data;

    // User presence handling
    uint16_t message_crc;
    uint8_t  user_presence;  // ledger_hid_u2f_user_presence_t
    const uint8_t *backup_message;
    uint16_t backup_message_length;

    // Context
    uint8_t os_context;

} ledger_hid_u2f_handle_t;

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ledger_hid_u2f_handle_t        ledger_hid_u2f_handle;
static uint8_t                        u2f_transport_packet_buffer[LEDGER_HID_U2F_EPIN_SIZE];
static usdb_ledger_hid_u2f_settings_t ledger_hid_u2f_settings;
static uint32_t                       ledger_hid_free_cid;

/* Exported variables --------------------------------------------------------*/
const usbd_end_point_info_t LEDGER_HID_U2F_end_point_info = {
    .ep_in_addr  = LEDGER_HID_U2F_EPIN_ADDR,
    .ep_in_size  = LEDGER_HID_U2F_EPIN_SIZE,
    .ep_out_addr = LEDGER_HID_U2F_EPOUT_ADDR,
    .ep_out_size = LEDGER_HID_U2F_EPOUT_SIZE,
    .ep_type     = USBD_EP_TYPE_INTR,
};

// clang-format off
const uint8_t LEDGER_HID_U2F_report_descriptor[34] = {
    0x06, 0xD0, 0xF1,                // Usage page      : FIDO_USAGE_PAGE
    0x09, 0x01,                      // Usage ID        : FIDO_USAGE_CTAPHID
    0xA1, 0x01,                      // Collection      : HID_Application

    // The Input report
    0x09, 0x20,                      // Usage ID        : FIDO_USAGE_DATA_IN
    0x15, 0x00,                      // Logical Minimum : 0
    0x26, 0xFF, 0x00,                // Logical Maximum : 255
    0x75, 0x08,                      // Report Size     : 8 bits
    0x95, LEDGER_HID_U2F_EPIN_SIZE,  // Report Count    : 64 fields
    0x81, 0x02,                      // Input           : Data, Variable, Absolute, No Wrap

    // The Output report
    0x09, 0x21,                      // Usage ID        : FIDO_USAGE_DATA_OUT
    0x15, 0x00,                      // Logical Minimum : 0
    0x26, 0xFF, 0x00,                // Logical Maximum : 255
    0x75, 0x08,                      // Report Size     : 8 bits
    0x95, LEDGER_HID_U2F_EPOUT_SIZE, // Report Count    : 64 fields
    0x91, 0x02,                      // Output          : Data, Variable, Absolute, No Wrap

    0xC0                             // Collection      : end
};

const uint8_t LEDGER_HID_U2F_descriptors[32] = {
    /************** Interface descriptor ******************************/
    USB_LEN_IF_DESC,                          // bLength
    USB_DESC_TYPE_INTERFACE,                  // bDescriptorType    : interface
    0x00,                                     // bInterfaceNumber   : 0 (dynamic)
    0x00,                                     // bAlternateSetting  : 0
    0x02,                                     // bNumEndpoints      : 2
    0x03,                                     // bInterfaceClass    : HID
    0x01,                                     // bInterfaceSubClass : boot
    0x01,                                     // bInterfaceProtocol : keyboard
    USBD_IDX_PRODUCT_STR,                     // iInterface         :

    /************** HID descriptor ************************************/
    0x09,                                     // bLength:
    HID_DESCRIPTOR_TYPE,                      // bDescriptorType : HID
    0x11,                                     // bcdHID
    0x01,                                     // bcdHID          : V1.11
    0x21,                                     // bCountryCode    : US
    0x01,                                     // bNumDescriptors : 1
    HID_REPORT_DESCRIPTOR_TYPE,               // bDescriptorType : report
    sizeof(LEDGER_HID_U2F_report_descriptor), // wItemLength
    0x00,

    /************** Endpoint descriptor *******************************/
    USB_LEN_EP_DESC,                          // bLength
    USB_DESC_TYPE_ENDPOINT,                   // bDescriptorType
    LEDGER_HID_U2F_EPIN_ADDR,                 // bEndpointAddress
    USBD_EP_TYPE_INTR,                        // bmAttributes
    LEDGER_HID_U2F_EPIN_SIZE,                 // wMaxPacketSize
    0x00,                                     // wMaxPacketSize
    0x01,                                     // bInterval

    /************** Endpoint descriptor *******************************/
    USB_LEN_EP_DESC,                          // bLength
    USB_DESC_TYPE_ENDPOINT,                   // bDescriptorType
    LEDGER_HID_U2F_EPOUT_ADDR,                // bEndpointAddress
    USBD_EP_TYPE_INTR,                        // bmAttributes
    LEDGER_HID_U2F_EPOUT_SIZE,                // wMaxPacketSize
    0x00,                                     // wMaxPacketSize
    0x01,                                     // bInterval
};
// clang-format on

const usbd_class_info_t USBD_LEDGER_HID_U2F_class_info = {
    .type = USBD_LEDGER_CLASS_HID_U2F,

    .end_point = &LEDGER_HID_U2F_end_point_info,

    .init     = USBD_LEDGER_HID_U2F_init,
    .de_init  = USBD_LEDGER_HID_U2F_de_init,
    .setup    = USBD_LEDGER_HID_U2F_setup,
    .data_in  = USBD_LEDGER_HID_U2F_data_in,
    .data_out = USBD_LEDGER_HID_U2F_data_out,

    .send_packet = USBD_LEDGER_HID_U2F_send_message,
    .is_busy     = USBD_LEDGER_HID_U2F_is_busy,

    .data_ready = USBD_LEDGER_HID_U2F_data_ready,

    .setting = USBD_LEDGER_HID_U2F_setting,

    .interface_descriptor      = LEDGER_HID_U2F_descriptors,
    .interface_descriptor_size = sizeof(LEDGER_HID_U2F_descriptors),

    .interface_association_descriptor      = NULL,
    .interface_association_descriptor_size = 0,

    .bos_descriptor      = NULL,
    .bos_descriptor_size = 0,

    .cookie = &ledger_hid_u2f_handle,
};

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
USBD_StatusTypeDef USBD_LEDGER_HID_U2F_init(USBD_HandleTypeDef *pdev, void *cookie)
{
    if (!cookie) {
        return USBD_FAIL;
    }

    UNUSED(pdev);

    ledger_hid_u2f_handle_t *handle = (ledger_hid_u2f_handle_t *) PIC(cookie);

    memset(handle, 0, sizeof(ledger_hid_u2f_handle_t));

    memset(&handle->transport_data, 0, sizeof(handle->transport_data));

    handle->transport_data.error = CTAP1_ERR_SUCCESS;

    handle->transport_data.rx_message_buffer      = USBD_LEDGER_io_buffer;
    handle->transport_data.rx_message_buffer_size = sizeof(USBD_LEDGER_io_buffer);

    handle->message_crc   = 0;
    handle->user_presence = LEDGER_HID_U2F_USER_PRESENCE_IDLE;

    handle->os_context = 0;

    U2F_TRANSPORT_init(&handle->transport_data, U2F_TRANSPORT_TYPE_USB_HID);

    USBD_LL_PrepareReceive(pdev, LEDGER_HID_U2F_EPOUT_ADDR, NULL, LEDGER_HID_U2F_EPOUT_SIZE);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_HID_U2F_de_init(USBD_HandleTypeDef *pdev, void *cookie)
{
    UNUSED(pdev);
    UNUSED(cookie);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_HID_U2F_setup(USBD_HandleTypeDef *pdev, void *cookie, USBD_SetupReqTypedef *req)
{
    if (!pdev || !cookie || !req) {
        return USBD_FAIL;
    }

    uint8_t                  ret    = USBD_OK;
    ledger_hid_u2f_handle_t *handle = (ledger_hid_u2f_handle_t *) PIC(cookie);

    uint8_t request = req->bmRequest & (USB_REQ_TYPE_MASK | USB_REQ_RECIPIENT_MASK);

    // HID Standard Requests
    if (request == (USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_INTERFACE)) {
        switch (req->bRequest) {
            case USB_REQ_GET_STATUS:
                if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                    uint16_t status_info = 0x0000;
                    USBD_CtlSendData(pdev, (uint8_t *) (void *) &status_info, sizeof(status_info));
                }
                else {
                    ret = USBD_FAIL;
                }
                break;

            case USB_REQ_GET_DESCRIPTOR:
                if (req->wValue == ((uint16_t) (HID_DESCRIPTOR_TYPE << 8))) {
                    USBD_CtlSendData(
                        pdev,
                        (uint8_t *) PIC(&LEDGER_HID_U2F_descriptors[USB_LEN_IF_DESC]),
                        MIN(LEDGER_HID_U2F_descriptors[USB_LEN_IF_DESC], req->wLength));
                }
                else if (req->wValue == ((uint16_t) (HID_REPORT_DESCRIPTOR_TYPE << 8))) {
                    USBD_CtlSendData(pdev,
                                     (uint8_t *) PIC(LEDGER_HID_U2F_report_descriptor),
                                     MIN(sizeof(LEDGER_HID_U2F_report_descriptor), req->wLength));
                }
                else {
                    ret = USBD_FAIL;
                }
                break;

            case USB_REQ_GET_INTERFACE:
                if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                    USBD_CtlSendData(pdev, &handle->alt_setting, sizeof(handle->alt_setting));
                }
                else {
                    ret = USBD_FAIL;
                }
                break;

            case USB_REQ_SET_INTERFACE:
                if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                    handle->alt_setting = (uint8_t) (req->wValue);
                }
                else {
                    ret = USBD_FAIL;
                }
                break;

            case USB_REQ_CLEAR_FEATURE:
                break;

            default:
                ret = USBD_FAIL;
                break;
        }
    }
    // HID Class-Specific Requests
    else if (request == (USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_INTERFACE)) {
        switch (req->bRequest) {
            case REQ_SET_PROTOCOL:
                handle->protocol = (uint8_t) (req->wValue);
                break;

            case REQ_GET_PROTOCOL:
                USBD_CtlSendData(pdev, &handle->protocol, sizeof(handle->protocol));
                break;

            case REQ_SET_IDLE:
                handle->idle_state = (uint8_t) (req->wValue >> 8);
                break;

            case REQ_GET_IDLE:
                USBD_CtlSendData(pdev, &handle->idle_state, sizeof(handle->idle_state));
                break;

            default:
                ret = USBD_FAIL;
                break;
        }
    }
    else {
        ret = USBD_FAIL;
    }

    return ret;
}

USBD_StatusTypeDef USBD_LEDGER_HID_U2F_ep0_rx_ready(USBD_HandleTypeDef *pdev, void *cookie)
{
    UNUSED(pdev);
    UNUSED(cookie);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_HID_U2F_data_in(USBD_HandleTypeDef *pdev, void *cookie, uint8_t ep_num)
{
    if (!cookie) {
        return USBD_FAIL;
    }

    UNUSED(ep_num);

    ledger_hid_u2f_handle_t *handle = (ledger_hid_u2f_handle_t *) PIC(cookie);

    if (handle->transport_data.tx_message_buffer) {
        U2F_TRANSPORT_tx(&handle->transport_data, 0, NULL, 0, u2f_transport_packet_buffer, sizeof(u2f_transport_packet_buffer));
        if (handle->transport_data.tx_packet_length) {
            handle->state = LEDGER_HID_U2F_STATE_BUSY;
            USBD_LL_Transmit(pdev,
                             LEDGER_HID_U2F_EPIN_ADDR,
                             u2f_transport_packet_buffer,
                             LEDGER_HID_U2F_EPIN_SIZE,
                             0);
        }
    }
    if (!handle->transport_data.tx_message_buffer) {
        handle->transport_data.tx_packet_length = 0;
        handle->state                           = LEDGER_HID_U2F_STATE_IDLE;
        if ((handle->transport_data.state == U2F_STATE_CMD_PROCESSING)
            || (handle->transport_data.state == U2F_STATE_CMD_PROCESSING_CANCEL)) {
            handle->transport_data.state = U2F_STATE_IDLE;
        }
    }

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_HID_U2F_data_out(USBD_HandleTypeDef *pdev,
                                     void               *cookie,
                                     uint8_t             ep_num,
                                     uint8_t            *packet,
                                     uint16_t            packet_length)
{
    if (!pdev || !cookie || !packet) {
        return USBD_FAIL;
    }

    UNUSED(ep_num);

    ledger_hid_u2f_handle_t *handle = (ledger_hid_u2f_handle_t *) PIC(cookie);

    U2F_TRANSPORT_rx(&handle->transport_data, packet, packet_length);

    USBD_LL_PrepareReceive(pdev, LEDGER_HID_U2F_EPOUT_ADDR, NULL, LEDGER_HID_U2F_EPOUT_SIZE);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_HID_U2F_send_message(USBD_HandleTypeDef *pdev,
                                         void               *cookie,
                                         uint8_t             packet_type,
                                         const uint8_t      *message,
                                         uint16_t            message_length,
                                         uint32_t            timeout_ms)
{
    if (!pdev || !cookie || !message || !message_length) {
        return USBD_FAIL;
    }

    uint8_t                  ret    = USBD_OK;
    ledger_hid_u2f_handle_t *handle = (ledger_hid_u2f_handle_t *) PIC(cookie);

    uint8_t  cmd       = 0;
    const uint8_t *tx_buffer = message;
    uint16_t tx_length = message_length;
#ifndef HAVE_BOLOS
    const uint8_t status[2] = {0x69, 0x85};
#endif // !HAVE_BOLOS

    switch (packet_type) {
        case OS_IO_PACKET_TYPE_USB_U2F_HID_APDU:
            cmd = U2F_COMMAND_MSG;
// Cannot enable user presence handling in the OS, see OS issues/555 for more information
#ifndef HAVE_BOLOS
            if ((message_length == 2) && (message[0] == 0xFF) && (message[1] == 0xFF)) {
                tx_buffer = status;
                handle->user_presence = LEDGER_HID_U2F_USER_PRESENCE_ASKING;
            }
            else if (handle->user_presence == LEDGER_HID_U2F_USER_PRESENCE_ASKING) {
                if ((message_length != 2) || (message[0] != 0x69) || (message[1] != 0x85)) {
                    handle->user_presence         = LEDGER_HID_U2F_USER_PRESENCE_CONFIRMED;
                    handle->backup_message        = message;
                    handle->backup_message_length = message_length;
                    return USBD_OK;
                }
            }
#endif // !HAVE_BOLOS
            break;

        case OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR:
            cmd = U2F_COMMAND_HID_CBOR;
            break;

        case OS_IO_PACKET_TYPE_USB_U2F_HID_RAW:
            cmd       = message[0];
            tx_buffer = &message[1];
            tx_length = message_length - 1;
            break;

        default:
            return USBD_FAIL;
            break;
    }

    U2F_TRANSPORT_tx(&handle->transport_data, cmd, tx_buffer, tx_length, u2f_transport_packet_buffer, sizeof(u2f_transport_packet_buffer));

    if (handle->transport_data.tx_packet_length) {
        if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            if (handle->state == LEDGER_HID_U2F_STATE_IDLE) {
                if (handle->transport_data.tx_message_buffer) {
                    handle->state = LEDGER_HID_U2F_STATE_BUSY;
                }
                ret = USBD_LL_Transmit(pdev,
                                       LEDGER_HID_U2F_EPIN_ADDR,
                                       u2f_transport_packet_buffer,
                                       LEDGER_HID_U2F_EPIN_SIZE,
                                       timeout_ms);
            }
            else {
                ret = USBD_BUSY;
            }
        }
        else {
            ret = USBD_FAIL;
        }
    }
    else {
        ret = USBD_FAIL;
    }

    return ret;
}

bool USBD_LEDGER_HID_U2F_is_busy(void *cookie)
{
    ledger_hid_u2f_handle_t *handle = (ledger_hid_u2f_handle_t *) PIC(cookie);
    bool                     busy   = false;

    if (handle->state == LEDGER_HID_U2F_STATE_BUSY) {
        busy = true;
    }

    return busy;
}

int32_t USBD_LEDGER_HID_U2F_data_ready(USBD_HandleTypeDef *pdev,
                                       void               *cookie,
                                       uint8_t            *buffer,
                                       uint16_t            max_length)
{
    int32_t status = 0;
    uint8_t error_msg[2];

    if (!cookie || !buffer) {
        return -1;
    }

    ledger_hid_u2f_handle_t *handle = (ledger_hid_u2f_handle_t *) PIC(cookie);

    error_msg[0] = U2F_COMMAND_ERROR;

    if (handle->transport_data.error == CTAP1_ERR_OTHER) {
        handle->transport_data.error = CTAP1_ERR_SUCCESS;
        return status;
    }

    if (handle->transport_data.error != CTAP1_ERR_SUCCESS) {
        if (handle->transport_data.error == CTAP2_ERR_INVALID_CBOR) {
            error_msg[0] = handle->transport_data.rx_message_buffer[0];
        }
        error_msg[1]                 = handle->transport_data.error;
        handle->transport_data.error = CTAP1_ERR_SUCCESS;
        USBD_LEDGER_HID_U2F_send_message(
            pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error_msg, 2, 0);
        return status;
    }

    if (handle->transport_data.state != U2F_STATE_CMD_COMPLETE) {
        return status;
    }

    handle->transport_data.state = U2F_STATE_IDLE;

    switch (handle->transport_data.rx_message_buffer[0]) {
        case U2F_COMMAND_HID_INIT:
            // Mandatory command
            if (handle->transport_data.rx_message_length != 11) {
                error_msg[1] = CTAP1_ERR_INVALID_LENGTH;
                USBD_LEDGER_HID_U2F_send_message(
                    pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error_msg, 2, 0);
            }
            else {
                uint8_t tx_packet_buffer[18];
                tx_packet_buffer[0] = U2F_COMMAND_HID_INIT;
                // Nonce
                memmove(&tx_packet_buffer[1], &handle->transport_data.rx_message_buffer[3], 8);
                // CID
                if (handle->transport_data.cid == U2F_BROADCAST_CID) {
                    U4BE_ENCODE(tx_packet_buffer, 9, ledger_hid_free_cid);
                    ledger_hid_free_cid++;
                }
                else {
                    U4BE_ENCODE(tx_packet_buffer, 9, handle->transport_data.cid);
                }
                // Versions & capabilities
                tx_packet_buffer[13] = ledger_hid_u2f_settings.protocol_version;
                tx_packet_buffer[14] = ledger_hid_u2f_settings.major_device_version_number;
                tx_packet_buffer[15] = ledger_hid_u2f_settings.minor_device_version_number;
                tx_packet_buffer[16] = ledger_hid_u2f_settings.build_device_version_number;
                tx_packet_buffer[17] = ledger_hid_u2f_settings.capabilities_flag;
                USBD_LEDGER_HID_U2F_send_message(pdev,
                                                 cookie,
                                                 OS_IO_PACKET_TYPE_USB_U2F_HID_RAW,
                                                 tx_packet_buffer,
                                                 sizeof(tx_packet_buffer),
                                                 0);
            }
            break;

        case U2F_COMMAND_MSG:
            // Mandatory command in CTAP1
            // Optional command in CTAP2 by setting MSG not implemented capability flag or not
            if ((ledger_hid_u2f_settings.protocol_version >= 2)
                && (ledger_hid_u2f_settings.capabilities_flag & U2F_HID_CAPABILITY_NMSG)) {
                error_msg[1] = CTAP1_ERR_INVALID_COMMAND;
                USBD_LEDGER_HID_U2F_send_message(
                    pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error_msg, 2, 0);
            }
            else {
                // Check length
                // Only Extended encoding is supported
                // APDU_MIN_HEADER:
                //     Either short or extended encoding with Lc and Le omitted
                // APDU_MIN_HEADER + 1:
                //     Short encoding, with next byte either Le or Lc with the other one omitted
                //     There is no way to tell so no way to check the value
                //     but anyway the data length is 0
                //     Support this particular short encoding APDU as Fido Conformance Tool v1.7.0
                //     is using it even though spec requires that short encoding should not be used
                //     over HID.
                bool length_ok = true;
                if ((handle->transport_data.rx_message_length < APDU_MIN_HEADER)
                    || (handle->transport_data.rx_message_length == (APDU_MIN_HEADER + 2))) {
                    length_ok = false;
                }
                else if (handle->transport_data.rx_message_length == (APDU_MIN_HEADER + 3)) {
                    if (buffer[APDU_MIN_HEADER] != 0) {
                        // Short encoding or bad length
                        // We don't support short encoding
                        length_ok = false;
                    }
                    // Can't be short encoding as Lc = 0x00 would lead to invalid length
                    // so extended encoding and either:
                    // - Lc = 0x00 0x00 0x00 and Le is omitted
                    // - Lc omitted and Le = 0x00 0xyy 0xzz
                    // so no way to check the value
                }

#ifdef HAVE_BOLOS
                uint16_t crc = cx_crc16_update_internal(0,
                                                        handle->transport_data.rx_message_buffer,
                                                        handle->transport_data.rx_message_length);
#else   // !HAVE_BOLOS
                uint16_t crc = cx_crc16_update(0,
                                               handle->transport_data.rx_message_buffer,
                                               handle->transport_data.rx_message_length);
#endif  // !HAVE_BOLOS
                if (!length_ok) {
                    error_msg[0] = 0x67;
                    error_msg[1] = 0x00;
                    USBD_LEDGER_HID_U2F_send_message(
                        pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, error_msg, 2, 0);
                }
// Cannot enable user presence handling in the OS, see OS issues/555 for more information
#ifndef HAVE_BOLOS
                else if (handle->user_presence == LEDGER_HID_U2F_USER_PRESENCE_ASKING) {
                    if (handle->message_crc == crc) {
                        error_msg[0] = 0x69;
                        error_msg[1] = 0x85;
                        USBD_LEDGER_HID_U2F_send_message(
                            pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, error_msg, 2, 0);
                    }
                    else {
                        error_msg[1] = CTAP1_ERR_CHANNEL_BUSY;
                        USBD_LEDGER_HID_U2F_send_message(
                            pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error_msg, 2, 0);
                    }
                }
                else if (handle->user_presence == LEDGER_HID_U2F_USER_PRESENCE_CONFIRMED) {
                    // Send backup response
                    if (handle->message_crc == crc) {
                        USBD_LEDGER_HID_U2F_send_message(pdev,
                                                         cookie,
                                                         OS_IO_PACKET_TYPE_USB_U2F_HID_APDU,
                                                         handle->backup_message,
                                                         handle->backup_message_length,
                                                         0);
                    }
                    handle->user_presence = LEDGER_HID_U2F_USER_PRESENCE_IDLE;
                }
#endif // !HAVE_BOLOS
                else if (max_length < handle->transport_data.rx_message_length - 2) {
                    error_msg[1] = CTAP1_ERR_INVALID_LENGTH;
                    USBD_LEDGER_HID_U2F_send_message(
                        pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, error_msg, 2, 0);
                }
                else {
                    if (handle->transport_data.rx_message_length + 1 > max_length) {
                        status = -1;
                    } else {
                        buffer[0] = OS_IO_PACKET_TYPE_USB_U2F_HID_APDU;
                        memmove(&buffer[1],
                                &handle->transport_data.rx_message_buffer[3],
                                handle->transport_data.rx_message_length - 3);
                        handle->transport_data.state = U2F_STATE_CMD_PROCESSING;
                        status                       = handle->transport_data.rx_message_length - 2;
                        handle->message_crc          = crc;
                        handle->user_presence        = LEDGER_HID_U2F_USER_PRESENCE_IDLE;
                    }
                }
            }
            break;

        case U2F_COMMAND_HID_CBOR:
            // Optional command available in CTAP2 only
            if (!(ledger_hid_u2f_settings.capabilities_flag & U2F_HID_CAPABILITY_CBOR)) {
                error_msg[1] = CTAP1_ERR_INVALID_COMMAND;
                USBD_LEDGER_HID_U2F_send_message(
                    pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error_msg, 2, 0);
            }
            else if (max_length < handle->transport_data.rx_message_length - 2) {
                error_msg[1] = CTAP1_ERR_INVALID_LENGTH;
                USBD_LEDGER_HID_U2F_send_message(
                    pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, error_msg, 2, 0);
            }
            else {
                if (handle->transport_data.rx_message_length + 1 > max_length) {
                    status = -1;
                } else {
                    buffer[0] = OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR;
                    memmove(&buffer[1],
                            &handle->transport_data.rx_message_buffer[3],
                            handle->transport_data.rx_message_length - 3);
                    handle->transport_data.state = U2F_STATE_CMD_PROCESSING;
                    status                       = handle->transport_data.rx_message_length - 2;
                }
            }
            break;

        case U2F_COMMAND_HID_WINK:
            // Optional command by setting WINK capability flag or not
            if (!(ledger_hid_u2f_settings.capabilities_flag & U2F_HID_CAPABILITY_WINK)) {
                error_msg[1] = CTAP1_ERR_INVALID_COMMAND;
                USBD_LEDGER_HID_U2F_send_message(
                    pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error_msg, 2, 0);
            }
            else {
                // TODO_IO : process the wink command
            }
            break;

        case U2F_COMMAND_HID_CANCEL:
            // Mandatory command in CTAP2 only
            if (ledger_hid_u2f_settings.protocol_version < 2) {
                error_msg[1] = CTAP1_ERR_INVALID_COMMAND;
                USBD_LEDGER_HID_U2F_send_message(
                    pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error_msg, 2, 0);
            }
            else if (max_length < handle->transport_data.rx_message_length - 2) {
                error_msg[1] = CTAP1_ERR_INVALID_LENGTH;
                USBD_LEDGER_HID_U2F_send_message(
                    pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL, error_msg, 2, 0);
            }
            else {
                // CTAPHID_CANCEL is transmitted to the application.
                // It is up to the application do ignore it if
                // no CTAPHID_CBOR request is being processed.
                buffer[0] = OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL;
                memmove(&buffer[1],
                        &handle->transport_data.rx_message_buffer[3],
                        handle->transport_data.rx_message_length - 3);
                handle->transport_data.state = U2F_STATE_CMD_PROCESSING_CANCEL;
                status                       = handle->transport_data.rx_message_length - 2;
            }
            handle->transport_data.cid = U2F_FORBIDDEN_CID;
            break;

        case U2F_COMMAND_PING:
            handle->transport_data.rx_message_buffer[2] = U2F_COMMAND_PING;
            USBD_LEDGER_HID_U2F_send_message(pdev,
                                             cookie,
                                             OS_IO_PACKET_TYPE_USB_U2F_HID_RAW,
                                             &handle->transport_data.rx_message_buffer[2],
                                             handle->transport_data.rx_message_length - 2,
                                             0);
            break;

        default:
            error_msg[1] = CTAP1_ERR_INVALID_COMMAND;
            USBD_LEDGER_HID_U2F_send_message(
                pdev, cookie, OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, error_msg, 2, 0);
            break;
    }

    return status;
}

void USBD_LEDGER_HID_U2F_setting(uint32_t id, uint8_t *buffer, uint16_t length, void *cookie)
{
    if (!cookie || !buffer) {
        return;
    }

    if ((id == USBD_LEDGER_HID_U2F_SETTING_ID_VERSIONS) && (buffer) && (length == 4)) {
        ledger_hid_u2f_settings.protocol_version            = buffer[0];
        ledger_hid_u2f_settings.major_device_version_number = buffer[1];
        ledger_hid_u2f_settings.minor_device_version_number = buffer[2];
        ledger_hid_u2f_settings.build_device_version_number = buffer[3];
    }
    else if ((id == USBD_LEDGER_HID_U2F_SETTING_ID_CAPABILITIES_FLAG) && (buffer)
             && (length == 1)) {
        ledger_hid_u2f_settings.capabilities_flag = buffer[0];
    }
    else if ((id == USBD_LEDGER_HID_U2F_SETTING_ID_FREE_CID) && (buffer) && (length == 4)) {
        ledger_hid_free_cid = U4BE(buffer, 0);
    }
}

#endif  // HAVE_IO_U2F
