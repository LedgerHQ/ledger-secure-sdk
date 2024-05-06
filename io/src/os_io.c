
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_io.h"
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"
#include "seproxyhal_protocol.h"
#include "checks.h"

#ifdef HAVE_IO_USB
#include "usbd_ledger.h"
#include "usbd_ledger_hid_u2f.h"
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
#include "ble_ledger.h"
#endif  // HAVE_BLE

#ifdef HAVE_IO_U2F
#include "lcx_rng.h"
#endif  // HAVE_IO_U2F

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
#ifndef USE_OS_IO_STACK
static int process_itc_event(uint8_t *buffer_in, size_t buffer_in_length);
#endif  // !USE_OS_IO_STACK

/* Exported variables --------------------------------------------------------*/
#ifndef HAVE_LOCAL_APDU_BUFFER
// apdu buffer must hold a complete apdu to avoid troubles
unsigned char G_io_rx_buffer[OS_IO_BUFFER_SIZE + 1];
unsigned char G_io_tx_buffer[OS_IO_BUFFER_SIZE + 1];
#endif

#ifndef USE_OS_IO_STACK
unsigned char G_io_seph_buffer[OS_IO_SEPH_BUFFER_SIZE + 1];
#endif  // !USE_OS_IO_STACK

uint8_t G_io_syscall_flag;
#ifdef HAVE_IO_U2F
uint32_t G_io_u2f_free_cid;
#endif  // HAVE_IO_U2F

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
#ifndef USE_OS_IO_STACK
static int process_itc_event(uint8_t *buffer_in, size_t buffer_in_length)
{
    int status = buffer_in_length;

    switch (buffer_in[3]) {
#ifdef HAVE_BLE
        case ITC_IO_BLE_DISABLE_ADV:
            BLE_LEDGER_enable_advertising(0);
            status = 0;
            break;

        case ITC_IO_BLE_ENABLE_ADV:
            BLE_LEDGER_enable_advertising(1);
            status = 0;
            break;

        case ITC_IO_BLE_RESET_PAIRINGS:
            BLE_LEDGER_reset_pairings();
            status = 0;
            break;

        case ITC_IO_BLE_BLE_NAME_CHANGED:
            // Restart advertising
            BLE_LEDGER_name_changed();
            status = 0;
            break;

        case ITC_UX_ACCEPT_BLE_PAIRING:
            BLE_LEDGER_accept_pairing(buffer_in[4]);
            status = 0;
            break;
#endif  // HAVE_BLE
#ifdef HAVE_SE_BUTTON
        case ITC_BUTTON_STATE: {
            uint8_t tx_buff[4];
            tx_buff[0] = SEPROXYHAL_TAG_BUTTON_PUSH_EVENT;
            tx_buff[1] = 0;
            tx_buff[2] = 1;
            tx_buff[3] = buffer_in[4];
            os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, tx_buff, 4, NULL);
            status = 0;
            break;
        }
#endif  // HAVE_SE_BUTTON
        case ITC_FINGER_STATE: {
            uint8_t tx_buff[10];
            tx_buff[0] = SEPROXYHAL_TAG_FINGER_EVENT;
            tx_buff[1] = 0;
            tx_buff[2] = 7;
            memcpy(&tx_buff[3], &buffer_in[4], 7);
            os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, tx_buff, 10, NULL);
            status = 0;
            break;
        }

#ifdef HAVE_SE_TOUCH
#endif  // HAVE_SE_TOUCH

        default:
            break;
    }

    return status;
}
#endif  // !USE_OS_IO_STACK

/* Exported functions --------------------------------------------------------*/
int32_t os_io_init(uint8_t full)
{
#ifdef HAVE_BAGL
    io_seph_ux_init_button();
#endif

#if (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))
    os_io_seph_cmd_mcu_protect();
#endif  // (!HAVE_BOLOS && HAVE_MCU_PROTECT)

#if !defined(HAVE_BOLOS) && defined(HAVE_PENDING_REVIEW_SCREEN)
    check_audited_app();
#endif  // !HAVE_BOLOS && HAVE_PENDING_REVIEW_SCREEN

    if (full) {
#ifdef HAVE_IO_USB
        USBD_LEDGER_init();
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
        BLE_LEDGER_init();
#endif  // HAVE_BLE
    }
#ifdef HAVE_NFC
    // TODO_IO
#endif  // HAVE_NFC

    return 0;
}

int32_t os_io_de_init(void)
{
    return 0;
}

int32_t os_io_stop(void)
{
    return 0;
}

#ifndef USE_OS_IO_STACK
int os_io_start(os_io_init_t *init)
{
    if (!init) {
        return -1;
    }

    if (G_io_syscall_flag == init->syscall) {
        os_io_init(1);
    }
    else {
        os_io_init(0);
    }
    G_io_syscall_flag = init->syscall;

#ifdef HAVE_IO_USB
    USBD_LEDGER_start(init->usb.pid, init->usb.vid, init->usb.name, init->usb.class_mask);
#ifdef HAVE_IO_U2F
    if (!G_io_syscall_flag) {
        cx_rng((uint8_t *) &G_io_u2f_free_cid, sizeof(G_io_u2f_free_cid));
    }
    if (init->usb.class_mask & USBD_LEDGER_CLASS_HID_U2F) {
        uint8_t buffer[4];
        buffer[0] = init->u2f_settings.protocol_version;
        buffer[1] = init->u2f_settings.major_device_version_number;
        buffer[2] = init->u2f_settings.minor_device_version_number;
        buffer[3] = init->u2f_settings.build_device_version_number;
        USBD_LEDGER_setting(
            USBD_LEDGER_CLASS_HID_U2F, USBD_LEDGER_HID_U2F_SETTING_ID_VERSIONS, buffer, 4);
        buffer[0] = init->u2f_settings.capabilities_flag;
        USBD_LEDGER_setting(
            USBD_LEDGER_CLASS_HID_U2F, USBD_LEDGER_HID_U2F_SETTING_ID_CAPABILITIES_FLAG, buffer, 1);
        U4BE_ENCODE(buffer, 0, G_io_u2f_free_cid);
        USBD_LEDGER_setting(
            USBD_LEDGER_CLASS_HID_U2F, USBD_LEDGER_HID_U2F_SETTING_ID_FREE_CID, buffer, 4);
    }
#endif  // HAVE_IO_U2F
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
    BLE_LEDGER_start(init->ble.profile_mask);
#endif  // HAVE_BLE

    return 0;
}

int os_io_rx_evt(unsigned char *buffer, unsigned short buffer_max_length, unsigned int *timeout_ms)
{
    int      status = 0;
    uint16_t length = 0;

    status = os_io_seph_se_rx_event(
        G_io_seph_buffer, sizeof(G_io_seph_buffer), (unsigned int *) timeout_ms, true, 0);
    if (status == -1) {
        status = os_io_seph_cmd_general_status();
        if (status < 0) {
            return status;
        }
        status = os_io_seph_se_rx_event(
            G_io_seph_buffer, sizeof(G_io_seph_buffer), (unsigned int *) timeout_ms, true, 0);
    }
    if (status < 0) {
        return status;
    }
#ifndef USE_OS_IO_STACK
    if ((G_io_seph_buffer[0] == OS_IO_PACKET_TYPE_SEPH)
        && (G_io_seph_buffer[1] == SEPROXYHAL_TAG_ITC_EVENT)) {
        status = process_itc_event(&G_io_seph_buffer[1], status - 1) + 1;
    }
#endif  // !USE_OS_IO_STACK
    if (status > 0) {
        length = (uint16_t) status;
    }

    switch (G_io_seph_buffer[1]) {
#ifdef HAVE_IO_USB
        case SEPROXYHAL_TAG_USB_EVENT:
        case SEPROXYHAL_TAG_USB_EP_XFER_EVENT:
            status = USBD_LEDGER_rx_seph_evt(G_io_seph_buffer, length, buffer, buffer_max_length);
            break;
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
        case SEPROXYHAL_TAG_BLE_RECV_EVENT:
            status = BLE_LEDGER_rx_seph_evt(G_io_seph_buffer, length, buffer, buffer_max_length);
            break;
#endif  // HAVE_BLE

#ifdef HAVE_NFC
        case SEPROXYHAL_TAG_NFC_APDU_EVENT:
            if (length >= buffer_max_length - 1) {
                length = buffer_max_length - 1;
            }
            buffer[0] = OS_IO_PACKET_TYPE_NFC_APDU;
            memmove(&buffer[1], &G_io_seph_buffer[4], length);
            break;
#endif  // HAVE_NFC

        case SEPROXYHAL_TAG_CAPDU_EVENT:
            if (length >= buffer_max_length - 1) {
                length = buffer_max_length - 1;
            }
            buffer[0] = OS_IO_PACKET_TYPE_RAW_APDU;
            memmove(&buffer[1], &G_io_seph_buffer[4], length);
            status = length - 3;
            break;

        default:
            if (length >= buffer_max_length - 1) {
                length = buffer_max_length - 1;
            }
            memmove(buffer, G_io_seph_buffer, length);
            break;
    }

    return status;
}

int os_io_tx_cmd(uint8_t                     type,
                 const unsigned char *buffer PLENGTH(length),
                 unsigned short              length,
                 unsigned int               *timeout_ms)
{
    int status = 0;
    switch (type) {
#ifdef HAVE_IO_USB
        case OS_IO_PACKET_TYPE_USB_HID_APDU:  // TODO_IO test error code
            USBD_LEDGER_send(USBD_LEDGER_CLASS_HID, type, buffer, length, 0);
            break;
#ifdef HAVE_WEBUSB
        case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
            USBD_LEDGER_send(USBD_LEDGER_CLASS_WEBUSB, type, buffer, length, 0);
            break;
#endif  // HAVE_WEBUSB
#ifdef HAVE_IO_U2F
        case OS_IO_PACKET_TYPE_USB_U2F_HID_APDU:
        case OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR:
            PRINTF("OS_IO_PACKET_TYPE_USB_U2F_HID_APDU");
            USBD_LEDGER_send(USBD_LEDGER_CLASS_HID_U2F, type, buffer, length, 0);
            break;
#endif  // HAVE_IO_U2F
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
        case OS_IO_PACKET_TYPE_BLE_APDU:
            BLE_LEDGER_send(buffer, length, 0);
            break;
#endif  // HAVE_BLE

        case OS_IO_PACKET_TYPE_RAW_APDU:
            os_io_seph_cmd_raw_apdu((const uint8_t *) buffer, length);
            break;

        case OS_IO_PACKET_TYPE_SEPH:
            status = os_io_seph_tx(buffer, length, (unsigned int *) timeout_ms);
            if (status == -1) {
                status = os_io_seph_se_rx_event(G_io_seph_buffer,
                                                sizeof(G_io_seph_buffer),
                                                (unsigned int *) timeout_ms,
                                                false,
                                                OS_IO_FLAG_TOTO);
                if (status >= 0) {
                    status = os_io_seph_tx(buffer, length, NULL);
                }
            }
            break;

        default:
            break;
    }

    return status;
}
#endif  // !USE_OS_IO_STACK
