/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usbd_ledger.h"
#include "usbd_ledger_hid.h"
#include "usbd_ledger_hid_kbd.h"
#include "usbd_ledger_hid_u2f.h"
#include "usbd_ledger_webusb.h"
#include "usbd_ledger_cdc.h"
#include "os_io_seph_cmd.h"
#include "seproxyhal_protocol.h"

#pragma GCC diagnostic ignored "-Wcast-qual"

/* Private enumerations ------------------------------------------------------*/

/* Private defines------------------------------------------------------------*/
#define USBD_BLUE_PRODUCT_STRING       ("Blue")
#define USBD_NANOS_PRODUCT_STRING      ("Nano S")
#define USBD_NANOX_PRODUCT_STRING      ("Nano X")
#define USBD_NANOS_PLUS_PRODUCT_STRING ("Nano S+")
#define USBD_STAX_PRODUCT_STRING       ("Stax")

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    usbd_ledger_product_e product;
    uint16_t              vid;
    uint16_t              pid;
    char                  name[20];
    uint8_t               bcdusb;
    uint8_t               usbd_iad;

    uint8_t nb_of_class;
    usbd_class_info_t *class[USBD_MAX_NUM_INTERFACES];
    uint16_t classes;

    USBD_HandleTypeDef usbd_handle;
    uint8_t            dev_state;

    uint16_t usb_ep_xfer_len[IO_USB_MAX_ENDPOINTS];
    uint8_t *usb_ep_xfer_buffer[IO_USB_MAX_ENDPOINTS];
} usbd_ledger_data_t;

#ifdef HAVE_PRINTF
#define DEBUG PRINTF
// #define DEBUG(...)
#else  // !HAVE_PRINTF
#define DEBUG(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static uint8_t init(USBD_HandleTypeDef *pdev, uint8_t cfg_idx);
static uint8_t de_init(USBD_HandleTypeDef *pdev, uint8_t cfg_idx);
static uint8_t setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t data_in(USBD_HandleTypeDef *pdev, uint8_t ep_num);
static uint8_t data_out(USBD_HandleTypeDef *pdev, uint8_t ep_num);
static uint8_t ep0_rx_ready(USBD_HandleTypeDef *pdev);

static uint8_t *get_cfg_desc(uint16_t *length);
static uint8_t *get_dev_qualifier_desc(uint16_t *length);
static uint8_t *get_bos_desc(USBD_SpeedTypeDef speed, uint16_t *length);

static void forge_configuration_descriptor(void);
static void forge_bos_descriptor(void);

/* Exported variables --------------------------------------------------------*/
uint8_t USBD_LEDGER_io_buffer[OS_IO_BUFFER_SIZE + 1];

/* Private variables ---------------------------------------------------------*/
static const USBD_ClassTypeDef USBD_LEDGER_CLASS = {
    init,                    // Init
    de_init,                 // DeInit
    setup,                   // Setup
    NULL,                    // EP0_TxSent
    ep0_rx_ready,            // EP0_RxReady
    data_in,                 // DataIn
    data_out,                // DataOut
    NULL,                    // SOF
    NULL,                    // IsoINIncomplete
    NULL,                    // IsoOUTIncomplete
    get_cfg_desc,            // GetHSConfigDescriptor
    get_cfg_desc,            // GetFSConfigDescriptor
    get_cfg_desc,            // GetOtherSpeedConfigDescriptor
    get_dev_qualifier_desc,  // GetDeviceQualifierDescriptor
};

static const uint8_t interface_descriptor[USB_LEN_CFG_DESC] = {
    USB_LEN_CFG_DESC,             // bLength
    USB_DESC_TYPE_CONFIGURATION,  // bDescriptorType
    0x00,                         // wTotalLength (dynamic)
    0x00,                         // wTotalLength (dynamic)
    0x00,                         // bNumInterfaces (dynamic)
    0x01,                         // bConfigurationValue
    USBD_IDX_PRODUCT_STR,         // iConfiguration
    0xC0,                         // bmAttributes: self powered
    0x32,                         // bMaxPower: 100 mA
};

static const uint8_t device_qualifier_decriptor[USB_LEN_DEV_QUALIFIER_DESC] = {
    USB_LEN_DEV_QUALIFIER_DESC,      // bLength
    USB_DESC_TYPE_DEVICE_QUALIFIER,  // bDescriptorType
    0x00,                            // bcdUSB
    0x02,                            // bcdUSB
    0x00,                            // bDeviceClass
    0x00,                            // bDeviceSubClass
    0x00,                            // bDeviceProtocol
    0x40,                            // bMaxPacketSize0
    0x01,                            // bNumConfigurations
    0x00,                            // bReserved
};

static const uint8_t bos_descriptor[] = {
    0x05,               // bLength
    USB_DESC_TYPE_BOS,  // bDescriptorType
    0x00,               // wTotalLength (dynamic)
    0x00,               // wTotalLength (dynamic)
    0x00,               // bNumDeviceCaps (dynamic)
};

static usbd_ledger_data_t usbd_ledger_data;

static uint8_t  usbd_ledger_descriptor[MAX_DESCRIPTOR_SIZE];
static uint16_t usbd_ledger_descriptor_size;

/* Private functions ---------------------------------------------------------*/
static uint8_t init(USBD_HandleTypeDef *pdev, uint8_t cfg_idx)
{
    uint8_t                ret            = USBD_OK;
    uint8_t                index          = 0;
    usbd_end_point_info_t *end_point_info = NULL;

    UNUSED(cfg_idx);
    pdev->pClassData = &usbd_ledger_data;

    usbd_class_info_t *class_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        class_info = usbd_ledger_data.class[index];
        if (class_info) {
            end_point_info = (usbd_end_point_info_t *) PIC(class_info->end_point);

            // Open EP IN
            if (end_point_info->ep_in_addr != 0xFF) {
                USBD_LL_OpenEP(pdev,
                               end_point_info->ep_in_addr,
                               end_point_info->ep_type,
                               end_point_info->ep_in_size);
                pdev->ep_in[end_point_info->ep_in_addr & 0x0F].is_used = 1;
            }

            // Open EP OUT
            if (end_point_info->ep_out_addr != 0xFF) {
                USBD_LL_OpenEP(pdev,
                               end_point_info->ep_out_addr,
                               end_point_info->ep_type,
                               end_point_info->ep_out_size);
                pdev->ep_out[end_point_info->ep_out_addr & 0x0F].is_used = 1;
            }

            if (class_info->init) {
                ret = ((usbd_class_init_t) PIC(class_info->init))(pdev, class_info->cookie);
            }
        }
    }
    usbd_ledger_data.dev_state = USBD_STATE_DEFAULT;

    return ret;
}

static uint8_t de_init(USBD_HandleTypeDef *pdev, uint8_t cfg_idx)
{
    uint8_t                ret            = USBD_OK;
    uint8_t                index          = 0;
    usbd_end_point_info_t *end_point_info = NULL;

    UNUSED(cfg_idx);

    usbd_class_info_t *class_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        class_info = usbd_ledger_data.class[index];
        if (class_info) {
            end_point_info = (usbd_end_point_info_t *) PIC(class_info->end_point);

            // Close EP IN
            if (end_point_info->ep_in_addr != 0xFF) {
                USBD_LL_CloseEP(pdev, end_point_info->ep_in_addr);
                pdev->ep_in[end_point_info->ep_in_addr & 0x0F].is_used = 0;
            }

            // Close EP OUT
            if (end_point_info->ep_out_addr != 0xFF) {
                USBD_LL_CloseEP(pdev, end_point_info->ep_out_addr);
                pdev->ep_out[end_point_info->ep_out_addr & 0x0F].is_used = 0;
            }

            if (class_info->de_init) {
                ret = ((usbd_class_de_init_t) PIC(class_info->de_init))(pdev, class_info->cookie);
            }
        }
    }

    pdev->pClassData = NULL;

    return ret;
}

static uint8_t setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    uint8_t  ret   = USBD_FAIL;
    uint16_t index = req->wIndex;

    if (index < usbd_ledger_data.nb_of_class) {
        usbd_class_info_t *class_info = usbd_ledger_data.class[index];
        ret = ((usbd_class_setup_t) PIC(class_info->setup))(pdev, class_info->cookie, req);
    }
    else {
        ret = USBD_FAIL;
        USBD_CtlError(pdev, req);
    }

    return ret;
}

static uint8_t data_in(USBD_HandleTypeDef *pdev, uint8_t ep_num)
{
    uint8_t ret   = USBD_OK;
    uint8_t index = 0;

    usbd_class_info_t     *class_info     = NULL;
    usbd_end_point_info_t *end_point_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        class_info     = usbd_ledger_data.class[index];
        end_point_info = (usbd_end_point_info_t *) PIC(class_info->end_point);
        if (((end_point_info->ep_in_addr & 0x7F) == (ep_num & 0x7F)) && (class_info->data_in)) {
            ret = ((usbd_class_data_in_t) PIC(class_info->data_in))(
                pdev, class_info->cookie, ep_num);
        }
    }

    return ret;
}

static uint8_t data_out(USBD_HandleTypeDef *pdev, uint8_t ep_num)
{
    uint8_t ret   = USBD_OK;
    uint8_t index = 0;

    usbd_class_info_t     *class_info     = NULL;
    usbd_end_point_info_t *end_point_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        class_info     = usbd_ledger_data.class[index];
        end_point_info = (usbd_end_point_info_t *) PIC(class_info->end_point);
        if (((end_point_info->ep_out_addr & 0x7F) == (ep_num & 0x7F)) && (class_info->data_out)) {
            ret = ((usbd_class_data_out_t) PIC(class_info->data_out))(
                pdev,
                class_info->cookie,
                ep_num,
                usbd_ledger_data.usb_ep_xfer_buffer[ep_num & 0x7F],
                usbd_ledger_data.usb_ep_xfer_len[ep_num & 0x7F]);
        }
    }

    return ret;
}

static uint8_t ep0_rx_ready(USBD_HandleTypeDef *pdev)
{
    UNUSED(pdev);

    return USBD_OK;
}

static uint8_t *get_cfg_desc(uint16_t *length)
{
    forge_configuration_descriptor();
    *length = usbd_ledger_descriptor_size;
    return usbd_ledger_descriptor;
}

static uint8_t *get_dev_qualifier_desc(uint16_t *length)
{
    *length = sizeof(device_qualifier_decriptor);
    return (uint8_t *) PIC(device_qualifier_decriptor);
}

static uint8_t *get_bos_desc(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    forge_bos_descriptor();
    *length = usbd_ledger_descriptor_size;
    return usbd_ledger_descriptor;
}

static void forge_configuration_descriptor(void)
{
    uint8_t offset = 0;
    uint8_t index  = 0;

    memset(usbd_ledger_descriptor, 0, sizeof(usbd_ledger_descriptor));

    memcpy(&usbd_ledger_descriptor[offset], interface_descriptor, sizeof(interface_descriptor));
    offset += sizeof(interface_descriptor);

    usbd_class_info_t *class_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        if (usbd_ledger_data.class[index]) {
            class_info = usbd_ledger_data.class[index];
            // Interface Association Descriptor
            if (class_info->interface_association_descriptor) {
                memcpy(&usbd_ledger_descriptor[offset],
                       PIC(class_info->interface_association_descriptor),
                       class_info->interface_association_descriptor_size);
                usbd_ledger_descriptor[offset + 2] = index;
                offset += class_info->interface_association_descriptor_size;
            }
            // Interface Descriptor
            memcpy(&usbd_ledger_descriptor[offset],
                   PIC(class_info->interface_descriptor),
                   class_info->interface_descriptor_size);
            usbd_ledger_descriptor[offset + 2] = index;
            if (class_info->type == USBD_LEDGER_CLASS_CDC_CONTROL) {
                usbd_ledger_descriptor[offset + 18] = index + 1;
                usbd_ledger_descriptor[offset + 26] = index;
                usbd_ledger_descriptor[offset + 27] = index + 1;
            }
            offset += class_info->interface_descriptor_size;
            usbd_ledger_descriptor[4]++;
        }
    }

    usbd_ledger_descriptor[2]   = (uint8_t) (offset >> 0);
    usbd_ledger_descriptor[3]   = (uint8_t) (offset >> 8);
    usbd_ledger_descriptor_size = offset;
}

static void forge_bos_descriptor(void)
{
    uint8_t offset = 0;
    uint8_t index  = 0;

    memset(usbd_ledger_descriptor, 0, sizeof(usbd_ledger_descriptor));

    memcpy(&usbd_ledger_descriptor[offset], bos_descriptor, sizeof(bos_descriptor));
    offset += sizeof(bos_descriptor);

    usbd_class_info_t *class_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        class_info = usbd_ledger_data.class[index];
        if ((class_info) && (class_info->bos_descriptor)) {
            memcpy(&usbd_ledger_descriptor[offset],
                   PIC(class_info->bos_descriptor),
                   class_info->bos_descriptor_size);
            offset += class_info->bos_descriptor_size;
            usbd_ledger_descriptor[4]++;
        }
    }

    usbd_ledger_descriptor[2]   = (uint8_t) (offset >> 0);
    usbd_ledger_descriptor[3]   = (uint8_t) (offset >> 8);
    usbd_ledger_descriptor[4]   = 2;
    usbd_ledger_descriptor_size = offset;
}

/* Exported functions --------------------------------------------------------*/
void USBD_LEDGER_init(void)
{
    os_io_seph_cmd_usb_disconnect();
    memset(&usbd_ledger_data, 0, sizeof(usbd_ledger_data));
}

void USBD_LEDGER_start(uint16_t pid, uint16_t vid, char *name, uint16_t class_mask)
{
    DEBUG("USBD_LEDGER_start %04X:%04X (%04X)", pid, vid, class_mask);
    if (name) {
        DEBUG(" %s\n", name);
    }
    else {
        DEBUG("\n");
    }
    if ((usbd_ledger_data.classes != class_mask) || (pid && (usbd_ledger_data.pid != pid))
        || (vid && (usbd_ledger_data.vid != vid))
        || (name && strcmp(usbd_ledger_data.name, name))) {
        usbd_ledger_data.bcdusb   = 0x00;
        usbd_ledger_data.usbd_iad = 0;

        USBD_LEDGER_init();

        usbd_ledger_data.product = USBD_LEDGER_PRODUCT_BLUE;
        strlcpy(usbd_ledger_data.name, USBD_BLUE_PRODUCT_STRING, sizeof(usbd_ledger_data.name));
#if defined(TARGET_NANOS)
        usbd_ledger_data.product = USBD_LEDGER_PRODUCT_NANOS;
        strlcpy(usbd_ledger_data.name, USBD_NANOS_PRODUCT_STRING, sizeof(usbd_ledger_data.name));
#endif  // TARGET_NANOS
#if defined(TARGET_NANOX)
        usbd_ledger_data.product = USBD_LEDGER_PRODUCT_NANOX;
        strlcpy(usbd_ledger_data.name, USBD_NANOX_PRODUCT_STRING, sizeof(usbd_ledger_data.name));
#endif  // TARGET_NANOX
#if defined(TARGET_NANOS2)
        usbd_ledger_data.product = USBD_LEDGER_PRODUCT_NANOS_PLUS;
        strlcpy(
            usbd_ledger_data.name, USBD_NANOS_PLUS_PRODUCT_STRING, sizeof(usbd_ledger_data.name));
#endif  // TARGET_NANOX
#if defined(TARGET_FATSTACKS) || defined(TARGET_STAX)
        usbd_ledger_data.product = USBD_LEDGER_PRODUCT_STAX;
        strlcpy(usbd_ledger_data.name, USBD_STAX_PRODUCT_STRING, sizeof(usbd_ledger_data.name));
#endif  // TARGET_FATSTACKS || TARGET_STAX

        if (vid) {
            usbd_ledger_data.vid = vid;
        }
        else {
            usbd_ledger_data.vid = USBD_VID;
        }

        if (pid) {
            usbd_ledger_data.pid = pid;
        }
        else {
            usbd_ledger_data.pid = usbd_ledger_data.product | (uint16_t) class_mask;
        }

        if (name && !strlen(usbd_ledger_data.name)) {
            strlcpy(usbd_ledger_data.name, name, sizeof(usbd_ledger_data.name));
        }

        if (class_mask & USBD_LEDGER_CLASS_HID) {
            usbd_ledger_data.class[usbd_ledger_data.nb_of_class++]
                = (usbd_class_info_t *) PIC(&USBD_LEDGER_HID_class_info);
        }
#ifdef HAVE_USB_HIDKBD
        else if (class_mask & USBD_LEDGER_CLASS_HID_KBD) {
            usbd_ledger_data.class[usbd_ledger_data.nb_of_class++]
                = (usbd_class_info_t *) PIC(&USBD_LEDGER_HID_KBD_class_info);
        }
#endif  // HAVE_USB_HIDKBD
#ifdef HAVE_IO_U2F
        if (class_mask & USBD_LEDGER_CLASS_HID_U2F) {
            usbd_ledger_data.class[usbd_ledger_data.nb_of_class++]
                = (usbd_class_info_t *) PIC(&USBD_LEDGER_HID_U2F_class_info);
        }
#endif  // HAVE_IO_U2F
#ifdef HAVE_USB_CLASS_CCID
        if (class_mask & USBD_LEDGER_CLASS_CCID) {
            // usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] = NULL;
        }
#endif  // HAVE_USB_CLASS_CCID
#ifdef HAVE_WEBUSB
        if (class_mask & USBD_LEDGER_CLASS_WEBUSB) {
            usbd_ledger_data.bcdusb = 0x10;
            usbd_ledger_data.class[usbd_ledger_data.nb_of_class++]
                = (usbd_class_info_t *) PIC(&USBD_LEDGER_WEBUSB_class_info);
        }
#endif  // HAVE_WEBUSB
#ifdef HAVE_CDCUSB
        if (class_mask & USBD_LEDGER_CLASS_CDC) {
            usbd_ledger_data.class[usbd_ledger_data.nb_of_class++]
                = (usbd_class_info_t *) PIC(&USBD_LEDGER_CDC_Control_class_info);
            usbd_ledger_data.class[usbd_ledger_data.nb_of_class++]
                = (usbd_class_info_t *) PIC(&USBD_LEDGER_CDC_Data_class_info);
            usbd_ledger_data.usbd_iad = 1;
        }
#endif  // HAVE_CDCUSB

        USBD_DESC_init(usbd_ledger_data.name,
                       usbd_ledger_data.vid,
                       usbd_ledger_data.pid,
                       usbd_ledger_data.bcdusb,
                       usbd_ledger_data.usbd_iad,
                       get_bos_desc);
        USBD_Init(&usbd_ledger_data.usbd_handle, (USBD_DescriptorsTypeDef *) &LEDGER_Desc, 0);
        USBD_RegisterClass(&usbd_ledger_data.usbd_handle, (USBD_ClassTypeDef *) &USBD_LEDGER_CLASS);
        USBD_Start(&usbd_ledger_data.usbd_handle);
        usbd_ledger_data.classes = class_mask;
    }
}

void USBD_LEDGER_add_profile(const usbd_class_info_t *class_info, uint8_t bcdusb, uint8_t usbd_iad)
{
    usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] = (usbd_class_info_t *) PIC(class_info);
    usbd_ledger_data.bcdusb |= bcdusb;
    if (usbd_iad) {
        usbd_ledger_data.usbd_iad = usbd_iad;
    }

    USBD_DESC_init(usbd_ledger_data.name,
                   usbd_ledger_data.vid,
                   usbd_ledger_data.pid,
                   usbd_ledger_data.bcdusb,
                   usbd_ledger_data.usbd_iad,
                   get_bos_desc);
}

void USBD_LEDGER_rx_evt_reset(void)
{
    USBD_LL_SetSpeed(&usbd_ledger_data.usbd_handle, USBD_SPEED_FULL);
    USBD_LL_Reset(&usbd_ledger_data.usbd_handle);
}

void USBD_LEDGER_rx_evt_sof(void)
{
    USBD_LL_SOF(&usbd_ledger_data.usbd_handle);
}

void USBD_LEDGER_rx_evt_suspend(void)
{
    USBD_LL_Suspend(&usbd_ledger_data.usbd_handle);
}

void USBD_LEDGER_rx_evt_resume(void)
{
    USBD_LL_Resume(&usbd_ledger_data.usbd_handle);
}

void USBD_LEDGER_rx_evt_setup(uint8_t *buffer)
{
    USBD_LL_SetupStage(&usbd_ledger_data.usbd_handle, buffer);
}

void USBD_LEDGER_rx_evt_data_in(uint8_t ep_num, uint8_t *buffer)
{
    USBD_LL_DataInStage(&usbd_ledger_data.usbd_handle, ep_num, buffer);
}

void USBD_LEDGER_rx_evt_data_out(uint8_t ep_num, uint8_t *buffer, uint16_t length)
{
    usbd_ledger_data.usb_ep_xfer_len[ep_num & 0x7F]    = length;
    usbd_ledger_data.usb_ep_xfer_buffer[ep_num & 0x7F] = buffer;
    USBD_LL_DataOutStage(&usbd_ledger_data.usbd_handle, ep_num, buffer);
}

uint32_t USBD_LEDGER_send(uint8_t        class_type,
                          uint8_t        packet_type,
                          const uint8_t *packet,
                          uint16_t       packet_length,
                          uint32_t       timeout_ms)
{
    uint32_t status     = INVALID_PARAMETER;
    uint8_t  usb_status = USBD_OK;
    uint8_t  index      = 0;

    usbd_class_info_t *class_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        class_info = usbd_ledger_data.class[index];
        if (class_info->type == class_type) {
            usb_status
                = ((usbd_send_packet_t) PIC(class_info->send_packet))(&usbd_ledger_data.usbd_handle,
                                                                      class_info->cookie,
                                                                      packet_type,
                                                                      packet,
                                                                      packet_length,
                                                                      timeout_ms);
            break;
        }
    }

    if (usb_status == USBD_OK) {
        status = SWO_SUCCESS;
    }
    else if (usb_status == USBD_TIMEOUT) {
        status = TIMEOUT;
    }

    return status;
}

int32_t USBD_LEDGER_data_ready(uint8_t *buffer, uint16_t max_length)
{
    uint8_t index  = 0;
    int32_t status = 0;

    usbd_class_info_t *class_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        class_info = usbd_ledger_data.class[index];
        if (class_info->data_ready) {
            status = ((usbd_data_ready_t) PIC(class_info->data_ready))(
                &usbd_ledger_data.usbd_handle, class_info->cookie, buffer, max_length);
            if (status > 0) {
                return status;
            }
        }
    }

    return 0;
}

void USBD_LEDGER_setting(uint32_t class_id, uint32_t setting_id, uint8_t *buffer, uint16_t length)
{
    uint8_t index = 0;

    usbd_class_info_t *class_info = NULL;
    for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
        class_info = usbd_ledger_data.class[index];
        if ((class_info->type == class_id) && (class_info->setting)) {
            ((usbd_setting_t) PIC(class_info->setting))(
                setting_id, buffer, length, class_info->cookie);
        }
    }
}

int USBD_LEDGER_rx_seph_evt(uint8_t *seph_buffer,
                            uint16_t seph_buffer_length,
                            uint8_t *apdu_buffer,
                            uint16_t apdu_buffer_max_length)
{
    int status = 0;

    if (seph_buffer[1] == SEPROXYHAL_TAG_USB_EVENT) {
        switch (seph_buffer[4]) {
            case SEPROXYHAL_TAG_USB_EVENT_RESET:
                USBD_LEDGER_rx_evt_reset();
                break;

            case SEPROXYHAL_TAG_USB_EVENT_SOF:
                USBD_LEDGER_rx_evt_sof();
                break;

            case SEPROXYHAL_TAG_USB_EVENT_SUSPENDED:
                USBD_LEDGER_rx_evt_suspend();
                break;

            case SEPROXYHAL_TAG_USB_EVENT_RESUMED:
                USBD_LEDGER_rx_evt_resume();
                break;

            default:
                status = -1;
                break;
        }
    }
    else if (seph_buffer[1] == SEPROXYHAL_TAG_USB_EP_XFER_EVENT) {
        uint8_t  epnum  = seph_buffer[4] & 0x7F;
        uint16_t length = MIN(seph_buffer[6], seph_buffer_length - 6);

        switch (seph_buffer[5]) {
            case SEPROXYHAL_TAG_USB_EP_XFER_SETUP:
                USBD_LEDGER_rx_evt_setup(&seph_buffer[7]);
                break;

            case SEPROXYHAL_TAG_USB_EP_XFER_IN:
                if (epnum < IO_USB_MAX_ENDPOINTS) {
                    USBD_LEDGER_rx_evt_data_in(epnum, &seph_buffer[7]);
                }
                else {
                    status = -1;
                }
                break;

            case SEPROXYHAL_TAG_USB_EP_XFER_OUT:
                if (epnum < IO_USB_MAX_ENDPOINTS) {
                    USBD_LEDGER_rx_evt_data_out(epnum, &seph_buffer[7], length);
                    status = USBD_LEDGER_data_ready(apdu_buffer, apdu_buffer_max_length);
                }
                else {
                    status = -1;
                }
                break;

            default:
                status = -1;
                break;
        }
    }
    else {
        return -1;
    }

    return status;
}