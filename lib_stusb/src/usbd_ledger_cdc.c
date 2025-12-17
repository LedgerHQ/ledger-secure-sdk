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
#include "os_io_legacy.h"
#include "usbd_ioreq.h"
#include "usbd_ledger.h"
#include "usbd_ledger_cdc.h"

#ifdef HAVE_CDCUSB

/* Private enumerations ------------------------------------------------------*/
enum ledger_cdc_state_t {
    LEDGER_CDC_STATE_IDLE,
    LEDGER_CDC_STATE_BUSY,
};
/* Private defines------------------------------------------------------------*/
#if defined(TARGET_NANOS2)
#define LEDGER_CDC_DATA_EPIN_ADDR (0x83)
#else  // !TARGET_NANOS2
#define LEDGER_CDC_DATA_EPIN_ADDR (0x85)
#endif  // !TARGET_NANOS2
#define LEDGER_CDC_DATA_EPIN_SIZE (0x40)

#if defined(TARGET_NANOS2)
#define LEDGER_CDC_DATA_EPOUT_ADDR (0x03)
#else  // !TARGET_NANOS2
#define LEDGER_CDC_DATA_EPOUT_ADDR (0x05)
#endif  // !TARGET_NANOS2
#define LEDGER_CDC_DATA_EPOUT_SIZE (0x40)

#if defined(TARGET_NANOS2)
#ifdef HAVE_IO_U2F
#define LEDGER_CDC_CONTROL_EPIN_ADDR (0x82)
#else   // ! HAVE_IO_U2F
#define LEDGER_CDC_CONTROL_EPIN_ADDR (0x81)
#endif  // !HAVE_IO_U2F
#else   // !TARGET_NANOS2
#define LEDGER_CDC_CONTROL_EPIN_ADDR (0x84)
#endif  // !TARGET_NANOS2
#define LEDGER_CDC_CONTROL_EPIN_SIZE (0x08)

#define LEDGER_CDC_SET_LINE_CODING (0x20U)
#define LEDGER_CDC_GET_LINE_CODING (0x21U)

#define MAX_PACKET_SIZE (0x40)

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    uint32_t bitrate;
    uint8_t  format;
    uint8_t  paritytype;
    uint8_t  datatype;
} line_coding_t;

typedef struct {
    uint8_t       data[MAX_PACKET_SIZE];
    uint8_t       cmd_op_code;
    uint8_t       cmd_length;
    line_coding_t line_coding;
    uint8_t       alt_setting;
    uint8_t       tx_in_progress;
    uint16_t      rx_length;
} ledger_cdc_handle_t;

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void control(ledger_cdc_handle_t *handle,
                    uint8_t              cmd_opcode,
                    uint8_t             *buffer,
                    uint16_t             length);

/* Private variables ---------------------------------------------------------*/
static ledger_cdc_handle_t ledger_cdc_handle;

/* Exported variables --------------------------------------------------------*/

const usbd_end_point_info_t LEDGER_CDC_Data_end_point_info = {
    .ep_in_addr  = LEDGER_CDC_DATA_EPIN_ADDR,
    .ep_in_size  = LEDGER_CDC_DATA_EPIN_SIZE,
    .ep_out_addr = LEDGER_CDC_DATA_EPOUT_ADDR,
    .ep_out_size = LEDGER_CDC_DATA_EPOUT_SIZE,
    .ep_type     = USBD_EP_TYPE_BULK,
};

const uint8_t LEDGER_CDC_Data_descriptors[23] = {
    /************** Data interface descriptor *************************/
    USB_LEN_IF_DESC,          // bLength
    USB_DESC_TYPE_INTERFACE,  // bDescriptorType    : interface
    0x00,                     // bInterfaceNumber   : 0 (dynamic)
    0x00,                     // bAlternateSetting  : 0
    0x02,                     // bNumEndpoints      : 2
    0x0A,                     // bInterfaceClass    : CDC Data
    0x00,                     // bInterfaceSubClass : Unused
    0x00,                     // bInterfaceProtocol : None
    0x05,                     // iInterface         : CDC ACM Data

    /************** Endpoint descriptor *******************************/
    0x07,                        // bLength
    USB_DESC_TYPE_ENDPOINT,      // bDescriptorType
    LEDGER_CDC_DATA_EPOUT_ADDR,  // bEndpointAddress
    USBD_EP_TYPE_BULK,           // bmAttributes
    MAX_PACKET_SIZE,             // wMaxPacketSize
    0x00,                        // wMaxPacketSize
    0x00,                        // bInterval

    /************** Endpoint descriptor *******************************/
    0x07,                       // bLength
    USB_DESC_TYPE_ENDPOINT,     // bDescriptorType
    LEDGER_CDC_DATA_EPIN_ADDR,  // bEndpointAddress
    USBD_EP_TYPE_BULK,          // bmAttributes
    MAX_PACKET_SIZE,            // wMaxPacketSize
    0x00,                       // wMaxPacketSize
    0x00,                       // bInterval
};

const usbd_class_info_t USBD_LEDGER_CDC_Data_class_info = {
    .type = USBD_LEDGER_CLASS_CDC_DATA,

    .end_point = &LEDGER_CDC_Data_end_point_info,

    .init     = USBD_LEDGER_CDC_init,
    .de_init  = USBD_LEDGER_CDC_de_init,
    .setup    = USBD_LEDGER_CDC_setup,
    .data_in  = USBD_LEDGER_CDC_data_in,
    .data_out = USBD_LEDGER_CDC_data_out,

    .send_packet = USBD_LEDGER_CDC_send_packet,
    .is_busy     = USBD_LEDGER_CDC_is_busy,

    .data_ready = USBD_LEDGER_CDC_data_ready,

    .setting = NULL,

    .interface_descriptor      = LEDGER_CDC_Data_descriptors,
    .interface_descriptor_size = sizeof(LEDGER_CDC_Data_descriptors),

    .interface_association_descriptor      = NULL,
    .interface_association_descriptor_size = 0,

    .bos_descriptor      = NULL,
    .bos_descriptor_size = 0,

    .cookie = &ledger_cdc_handle,
};

const usbd_end_point_info_t LEDGER_CDC_Control_end_point_info = {
    .ep_in_addr  = LEDGER_CDC_CONTROL_EPIN_ADDR,
    .ep_in_size  = LEDGER_CDC_CONTROL_EPIN_SIZE,
    .ep_out_addr = 0xFF,
    .ep_out_size = 0,
    .ep_type     = USBD_EP_TYPE_INTR,
};

const uint8_t LEDGER_CDC_Control_descriptors[35] = {
    /************** Interface descriptor ******************************/
    USB_LEN_IF_DESC,          // bLength
    USB_DESC_TYPE_INTERFACE,  // bDescriptorType    : interface
    0x00,                     // bInterfaceNumber   : 0 (dynamic)
    0x00,                     // bAlternateSetting  : 0
    0x01,                     // bNumEndpoints      : 1
    0x02,                     // bInterfaceClass    : Communications
    0x02,                     // bInterfaceSubClass : Abstract (modem)
    0x00,                     // bInterfaceProtocol : None
    0x04,                     // iInterface         : CDC Abstract Control Model (ACM)

    /************** CDC Header Functional Descriptor ******************/
    0x05,  // bLength
    0x24,  // bDescriptorType    : CDC
    0x00,  // bDescriptorSubtype : Header
    0x10,  // bcdCDC             : Specification release number
    0x01,  // bcdCDC             : Specification release number

    /************** CDC Call Management Functional Descriptor *********/
    0x05,  // bLength
    0x24,  // bDescriptorType    : CDC
    0x01,  // bDescriptorSubtype : Call Management
    0x00,  // bmCapabilities     : none
    0x01,  // bDataInterface     : 0 (dynamic)

    /************** CDC ACM Functional Descriptor *********************/
    0x04,  // bLength
    0x24,  // bDescriptorType    : CDC
    0x01,  // bDescriptorSubtype : ACM
    0x02,  // bmCapabilities     : line coding and serial state

    /************** CDC Union Functional Descriptor *******************/
    0x05,  // bLength
    0x24,  // bDescriptorType    : CDC
    0x06,  // bDescriptorSubtype : Union
    0x00,  // bMasterInterface   : 0 (dynamic)
    0x01,  // bSlaveInterface    : 0 (dynamic)

    /************** Endpoint descriptor *******************************/
    0x07,                          // bLength
    USB_DESC_TYPE_ENDPOINT,        // bDescriptorType
    LEDGER_CDC_CONTROL_EPIN_ADDR,  // bEndpointAddress
    0x03,                          // bmAttributes       : Interrupt/No Sync/Data
    LEDGER_CDC_CONTROL_EPIN_SIZE,  // wMaxPacketSize
    0x00,                          // wMaxPacketSize
    0x10,                          // bInterval
};

const uint8_t LEDGER_CDC_Control_interface_association_descriptor[8] = {
    /************** Interface Association descriptor ******************/
    0x08,  // bLength
    0x0B,  // bDescriptorType    : Interface Association
    0x00,  // bFirstInterface    : 0 (dynamic)
    0x02,  // bInterfaceCount
    0x02,  // bFunctionClass     : Communications
    0x02,  // bFunctionSubClass  : Abstract (modem)
    0x00,  // bFunctionProtocol  : None
    0x04,  // iFunction          :
};

const usbd_class_info_t USBD_LEDGER_CDC_Control_class_info = {
    .type = USBD_LEDGER_CLASS_CDC_CONTROL,

    .end_point = &LEDGER_CDC_Control_end_point_info,

    .init     = USBD_LEDGER_CDC_cmd_init,
    .de_init  = USBD_LEDGER_CDC_de_init,
    .setup    = USBD_LEDGER_CDC_setup,
    .data_in  = USBD_LEDGER_CDC_data_in,
    .data_out = USBD_LEDGER_CDC_data_out,

    .send_packet = USBD_LEDGER_CDC_send_packet,
    .is_busy     = NULL,

    .data_ready = NULL,

    .setting = NULL,

    .interface_descriptor      = LEDGER_CDC_Control_descriptors,
    .interface_descriptor_size = sizeof(LEDGER_CDC_Control_descriptors),

    .interface_association_descriptor = LEDGER_CDC_Control_interface_association_descriptor,
    .interface_association_descriptor_size
    = sizeof(LEDGER_CDC_Control_interface_association_descriptor),

    .bos_descriptor      = NULL,
    .bos_descriptor_size = 0,

    .cookie = &ledger_cdc_handle,
};

/* Private functions ---------------------------------------------------------*/
static void control(ledger_cdc_handle_t *handle,
                    uint8_t              cmd_opcode,
                    uint8_t             *buffer,
                    uint16_t             length)
{
    UNUSED(length);

    switch (cmd_opcode) {
        case LEDGER_CDC_SET_LINE_CODING:
            handle->line_coding.bitrate    = (uint32_t) ((buffer[0] << 0) | (buffer[1] << 8)
                                                      | (buffer[2] << 16) | (buffer[3] << 24));
            handle->line_coding.format     = buffer[4];
            handle->line_coding.paritytype = buffer[5];
            handle->line_coding.datatype   = buffer[6];
            break;

        case LEDGER_CDC_GET_LINE_CODING:
            buffer[0] = (uint8_t) (handle->line_coding.bitrate >> 0);
            buffer[1] = (uint8_t) (handle->line_coding.bitrate >> 8);
            buffer[2] = (uint8_t) (handle->line_coding.bitrate >> 16);
            buffer[3] = (uint8_t) (handle->line_coding.bitrate >> 24);
            buffer[4] = handle->line_coding.format;
            buffer[5] = handle->line_coding.paritytype;
            buffer[6] = handle->line_coding.datatype;
            break;

        default:
            break;
    }
}

/* Exported functions --------------------------------------------------------*/
USBD_StatusTypeDef USBD_LEDGER_CDC_init(USBD_HandleTypeDef *pdev, void *cookie)
{
    if (!cookie) {
        return USBD_FAIL;
    }

    UNUSED(pdev);

    ledger_cdc_handle_t *handle = (ledger_cdc_handle_t *) PIC(cookie);

    memset(handle, 0, sizeof(ledger_cdc_handle_t));

    USBD_LL_PrepareReceive(pdev, LEDGER_CDC_DATA_EPOUT_ADDR, NULL, LEDGER_CDC_DATA_EPOUT_SIZE);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_CDC_cmd_init(USBD_HandleTypeDef *pdev, void *cookie)
{
    if (!cookie) {
        return USBD_FAIL;
    }

    UNUSED(pdev);
    pdev->ep_in[LEDGER_CDC_CONTROL_EPIN_ADDR & 0xFU].bInterval = 0x10;

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_CDC_de_init(USBD_HandleTypeDef *pdev, void *cookie)
{
    UNUSED(pdev);
    UNUSED(cookie);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_CDC_setup(USBD_HandleTypeDef   *pdev,
                                         void                 *cookie,
                                         USBD_SetupReqTypedef *req)
{
    if (!pdev || !cookie || !req) {
        return USBD_FAIL;
    }

    uint8_t              ret         = USBD_OK;
    ledger_cdc_handle_t *handle      = (ledger_cdc_handle_t *) PIC(cookie);
    uint16_t             status_info = 0x0000;
    uint16_t             length      = 0;

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_CLASS:
            if ((req->bmRequest & 0x80U) != 0U) {
                control(handle, req->bRequest, handle->data, req->wLength);
                length = MIN(7, req->wLength);
                (void) USBD_CtlSendData(pdev, handle->data, length);
            }
            else {
                handle->cmd_op_code = req->bRequest;
                handle->cmd_length  = (uint8_t) req->wLength;
                (void) USBD_CtlPrepareRx(pdev, handle->data, req->wLength);
            }
            break;

        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest) {
                case USB_REQ_GET_STATUS:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                        USBD_CtlSendData(
                            pdev, (uint8_t *) (void *) &status_info, sizeof(status_info));
                    }
                    else {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;
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
            break;

        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
    }
    return ret;

    return ret;
}

USBD_StatusTypeDef USBD_LEDGER_CDC_ep0_rx_ready(USBD_HandleTypeDef *pdev, void *cookie)
{
    UNUSED(pdev);
    UNUSED(cookie);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LEDGER_CDC_data_in(USBD_HandleTypeDef *pdev, void *cookie, uint8_t ep_num)
{
    USBD_StatusTypeDef status = USBD_FAIL;
    if (!pdev || !cookie) {
        goto error;
    }

    ledger_cdc_handle_t *handle = (ledger_cdc_handle_t *) PIC(cookie);

    if ((pdev->ep_in[ep_num].total_length > 0)
        && ((pdev->ep_in[ep_num].total_length % MAX_PACKET_SIZE) == 0)) {
        pdev->ep_in[ep_num].total_length = 0;
        handle->tx_in_progress           = LEDGER_CDC_STATE_BUSY;
        status                           = USBD_LL_Transmit(pdev, ep_num, NULL, 0, 0);
        handle->tx_in_progress           = LEDGER_CDC_STATE_IDLE;
    }
    else {
        handle->tx_in_progress = LEDGER_CDC_STATE_IDLE;
        status                 = USBD_OK;
    }

error:
    return status;
}

USBD_StatusTypeDef USBD_LEDGER_CDC_data_out(USBD_HandleTypeDef *pdev,
                                            void               *cookie,
                                            uint8_t             ep_num,
                                            uint8_t            *packet,
                                            uint16_t            packet_length)
{
    USBD_StatusTypeDef status = USBD_FAIL;
    if (!pdev || !cookie || !packet) {
        goto error;
    }

    ledger_cdc_handle_t *handle = (ledger_cdc_handle_t *) PIC(cookie);

    if (handle->rx_length  >= sizeof(USBD_LEDGER_io_buffer)) {
        goto error;
    }
    if (handle->rx_length + packet_length >= sizeof(USBD_LEDGER_io_buffer)) {
        memcpy(&USBD_LEDGER_io_buffer[handle->rx_length],
               packet,
               sizeof(USBD_LEDGER_io_buffer) - handle->rx_length);
        handle->rx_length = sizeof(USBD_LEDGER_io_buffer);
    }
    else {
        memcpy(&USBD_LEDGER_io_buffer[handle->rx_length], packet, packet_length);
        handle->rx_length += packet_length;
    }

    if (ep_num == LEDGER_CDC_DATA_EPOUT_ADDR) {
        status = USBD_LL_PrepareReceive(
            pdev, LEDGER_CDC_DATA_EPOUT_ADDR, NULL, LEDGER_CDC_DATA_EPOUT_SIZE);
    }

error:
    return status;
}

USBD_StatusTypeDef USBD_LEDGER_CDC_send_packet(USBD_HandleTypeDef *pdev,
                                               void               *cookie,
                                               uint8_t             packet_type,
                                               const uint8_t      *packet,
                                               uint16_t            packet_length,
                                               uint32_t            timeout_ms)
{
    USBD_StatusTypeDef ret = USBD_FAIL;
    if (!pdev || !cookie || !packet) {
        goto error;
    }

    UNUSED(packet_type);

    ledger_cdc_handle_t *handle = (ledger_cdc_handle_t *) PIC(cookie);

    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        if (handle->tx_in_progress == LEDGER_CDC_STATE_IDLE) {
            handle->tx_in_progress = LEDGER_CDC_STATE_BUSY;
            ret                    = USBD_LL_Transmit(
                pdev, LEDGER_CDC_DATA_EPIN_ADDR, packet, packet_length, timeout_ms);
        }
        else {
            ret = USBD_BUSY;
        }
    }
    else {
        ret = USBD_FAIL;
    }

error:
    return ret;
}

bool USBD_LEDGER_CDC_is_busy(void *cookie)
{
    ledger_cdc_handle_t *handle = (ledger_cdc_handle_t *) PIC(cookie);
    bool                 busy   = false;

    if (handle->tx_in_progress == LEDGER_CDC_STATE_BUSY) {
        busy = true;
    }

    return busy;
}

int32_t USBD_LEDGER_CDC_data_ready(USBD_HandleTypeDef *pdev,
                                   void               *cookie,
                                   uint8_t            *buffer,
                                   uint16_t            max_length)
{
    int32_t status = 0;

    UNUSED(pdev);
    UNUSED(max_length);

    if (!cookie || !buffer) {
        return -1;
    }

    ledger_cdc_handle_t *handle = (ledger_cdc_handle_t *) PIC(cookie);
    if ((handle->tx_in_progress == LEDGER_CDC_STATE_IDLE) && (handle->rx_length != 0)) {
        buffer[0] = OS_IO_PACKET_TYPE_AT_CMD;
        if (handle->rx_length < (max_length - 1) && (sizeof(USBD_LEDGER_io_buffer) >= handle->rx_length)) {
            memmove(&buffer[1], USBD_LEDGER_io_buffer, handle->rx_length);
            status = handle->rx_length + 1;
        } else {
            status = -1;
        }
        handle->rx_length = 0;
    }
    else {
        status = 0;
    }

    return status;
}

#endif  // HAVE_CDCUSB
