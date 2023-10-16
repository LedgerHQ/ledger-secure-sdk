
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_io.h"
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"
#include "seproxyhal_protocol.h"

#define CALVA_LOG(...) snprintf(((char *) 0xF0000000), 0x1000, __VA_ARGS__);
/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
#ifndef HAVE_LOCAL_APDU_BUFFER
// apdu buffer must hold a complete apdu to avoid troubles
unsigned char G_io_rx_buffer[IO_APDU_BUFFER_SIZE + 1];
unsigned char G_io_tx_buffer[IO_APDU_BUFFER_SIZE + 1];
#endif

unsigned char G_io_seph_rx_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B + 1];
uint16_t      G_io_seph_rx_buffer_length;

io_seph_app_t G_io_app;

uint8_t G_io_syscall_flag;

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
#ifdef HAVE_BLE
static int process_ux_ble_event(uint8_t *buffer_in, size_t buffer_in_length)
{
    int status = buffer_in_length;

    switch (buffer_in[3]) {
        case SEPROXYHAL_TAG_UX_CMD_BLE_DISABLE_ADV:
            BLE_LEDGER_enable_advertising(0);
            status = 0;
            break;

        case SEPROXYHAL_TAG_UX_CMD_BLE_ENABLE_ADV:
            BLE_LEDGER_enable_advertising(1);
            status = 0;
            break;

        case SEPROXYHAL_TAG_UX_CMD_BLE_RESET_PAIRINGS:
            BLE_LEDGER_reset_pairings();
            status = 0;
            break;

        case SEPROXYHAL_TAG_UX_CMD_BLE_NAME_CHANGED:
            // Restart advertising
            BLE_LEDGER_name_changed();
            status = 0;
            break;

        case SEPROXYHAL_TAG_UX_CMD_ACCEPT_PAIRING:
            BLE_LEDGER_accept_pairing(buffer_in[4]);
            status = 0;
            break;

        default:
            break;
    }

    return status;
}
#endif  // HAVE_BLE

/* Exported functions --------------------------------------------------------*/
int32_t os_io_init(uint8_t full)
{
    memset(&G_io_app, 0, sizeof(G_io_app));
    G_io_app.apdu_media = IO_APDU_MEDIA_NONE;

#ifdef HAVE_BAGL
    io_seph_ux_init_button();
#endif

#if (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))
    io_seph_cmd_mcu_protect();
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
    // TODO
#endif  // HAVE_NFC

#if !defined(HAVE_BOLOS) && defined(HAVE_PENDING_REVIEW_SCREEN)
    // check_audited_app(); // TODO
#endif  // !defined(HAVE_BOLOS) && defined(HAVE_PENDING_REVIEW_SCREEN)

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

#ifndef TUTU
int os_io_start(os_io_init_t *init)
{
    if (!init) {
        return -1;
    }

    if (G_io_syscall_flag == init->syscall) {
        io_seproxyhal_disable_io();
        os_io_init(1);
    }
    else {
        os_io_init(0);
    }
    G_io_syscall_flag = init->syscall;

#ifdef HAVE_IO_USB
    USBD_LEDGER_start(init->usb.pid, init->usb.vid, init->usb.name, init->usb.class_mask);
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

    if (!G_io_seph_rx_buffer_length) {
        status = os_io_seph_se_rx_event(
            G_io_seph_rx_buffer, sizeof(G_io_seph_rx_buffer), (unsigned int *) timeout_ms, true, 0);

        if (status == -1) {
            status = io_seph_cmd_general_status();
            if (status < 0) {
                return status;
            }
            status = os_io_seph_se_rx_event(G_io_seph_rx_buffer,
                                            sizeof(G_io_seph_rx_buffer),
                                            (unsigned int *) timeout_ms,
                                            true,
                                            0);
        }
        if (status < 0) {
            return status;
        }
        if ((G_io_seph_rx_buffer[0] == OS_IO_PACKET_TYPE_SEPH)
            && (G_io_seph_rx_buffer[1] == SEPROXYHAL_TAG_UX_EVENT)) {
            status = process_ux_ble_event(&G_io_seph_rx_buffer[1], status - 1) + 1;
        }
        if (status > 0) {
            length = (uint16_t) status;
        }
    }
    else {
        length                     = G_io_seph_rx_buffer_length;
        G_io_seph_rx_buffer_length = 0;
    }

    switch (G_io_seph_rx_buffer[1]) {
#ifdef HAVE_IO_USB
        case SEPROXYHAL_TAG_USB_EVENT:
        case SEPROXYHAL_TAG_USB_EP_XFER_EVENT:
            status
                = USBD_LEDGER_rx_seph_evt(G_io_seph_rx_buffer, length, buffer, buffer_max_length);
            break;
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
        case SEPROXYHAL_TAG_BLE_RECV_EVENT:
            status = BLE_LEDGER_rx_seph_evt(G_io_seph_rx_buffer, length, buffer, buffer_max_length);
            break;
#endif  // HAVE_BLE

#ifdef HAVE_NFC
        case SEPROXYHAL_TAG_NFC_APDU_EVENT:
            if (length >= buffer_max_length - 1) {
                length = buffer_max_length - 1;
            }
            buffer[0] = OS_IO_PACKET_TYPE_NFC_APDU;
            memmove(&buffer[1], &G_io_seph_rx_buffer[4], length);
            break;
#endif  // HAVE_NFC

        case SEPROXYHAL_TAG_CAPDU_EVENT:
            if (length >= buffer_max_length - 1) {
                length = buffer_max_length - 1;
            }
            buffer[0] = OS_IO_PACKET_TYPE_RAW_APDU;
            memmove(&buffer[1], &G_io_seph_rx_buffer[4], length);
            status = length - 3;
            break;

        default:
            if (length >= buffer_max_length - 1) {
                length = buffer_max_length - 1;
            }
            memmove(buffer, G_io_seph_rx_buffer, length);
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
        case OS_IO_PACKET_TYPE_USB_HID_APDU:  // TODO test error code
            USBD_LEDGER_send(USBD_LEDGER_CLASS_HID, buffer, length, 0);
            break;
#ifdef HAVE_WEBUSB
        case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
            USBD_LEDGER_send(USBD_LEDGER_CLASS_WEBUSB, buffer, length, 0);
            break;
#endif  // HAVE_WEBUSB
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
        case OS_IO_PACKET_TYPE_BLE_APDU:
            BLE_LEDGER_send(buffer, length, 0);
            break;
#endif  // HAVE_BLE

        case OS_IO_PACKET_TYPE_RAW_APDU:
            io_seph_cmd_raw_apdu((const uint8_t *) buffer, length);
            break;

        case OS_IO_PACKET_TYPE_SEPH:
            status = os_io_seph_tx(buffer, length, (unsigned int *) timeout_ms);
            if (status == -1) {
                status = os_io_seph_se_rx_event(G_io_seph_rx_buffer,
                                                sizeof(G_io_seph_rx_buffer),
                                                (unsigned int *) timeout_ms,
                                                false,
                                                0);
                if (status >= 0) {
                    G_io_seph_rx_buffer_length = status;  // TODO : process the rx packet?
                    status                     = os_io_seph_tx(buffer, length, NULL);
                }
            }
            break;

        default:
            break;
    }

    return status;
}
#endif  // TUTU
