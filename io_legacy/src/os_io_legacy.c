/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "os_helpers.h"
#include "os_io.h"
#include "os_apdu.h"
#include "os_io_default_apdu.h"
#include "os_io_legacy.h"

#ifdef HAVE_IO_USB
#include "usbd_ledger.h"
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
#include "ble_ledger.h"
#endif  // HAVE_BLE

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
io_seph_app_t G_io_app;

/* Private variables ---------------------------------------------------------*/

// Variable containing the type of the APDU response to send back.
static apdu_type_t io_os_legacy_apdu_type;

/* Private functions ---------------------------------------------------------*/
static io_apdu_media_t get_media_from_apdu_type(apdu_type_t apdu_type)
{
    if (apdu_type == APDU_TYPE_RAW) {
        return IO_APDU_MEDIA_RAW;
    }
    if (apdu_type == APDU_TYPE_USB_HID) {
        return IO_APDU_MEDIA_USB_HID;
    }
    if (apdu_type == APDU_TYPE_USB_WEBUSB) {
        return IO_APDU_MEDIA_USB_WEBUSB;
    }
    if (apdu_type == APDU_TYPE_USB_CCID) {
        return IO_APDU_MEDIA_USB_CCID;
    }
    if (apdu_type == APDU_TYPE_USB_U2F) {
        return IO_APDU_MEDIA_U2F;
    }
    if (apdu_type == APDU_TYPE_BLE) {
        return IO_APDU_MEDIA_BLE;
    }
    if (apdu_type == APDU_TYPE_NFC) {
        return IO_APDU_MEDIA_NFC;
    }
    return IO_APDU_MEDIA_NONE;
}

/* Exported functions --------------------------------------------------------*/

void io_seproxyhal_general_status(void)
{
    (void) os_io_seph_cmd_general_status();
}

void io_seproxyhal_se_reset(void)
{
    os_io_seph_cmd_se_reset();
    for (;;)
        ;
}

void io_seproxyhal_disable_io(void)
{
    os_io_stop();
}

void io_seproxyhal_io_heartbeat(void)
{
    uint16_t      err = 0x6601;
    unsigned char err_buffer[2];
    int           status = os_io_rx_evt(G_io_rx_buffer, sizeof(G_io_rx_buffer), NULL);

    err_buffer[0] = err >> 8;
    err_buffer[1] = err;

    if (status > 0) {
        switch (G_io_rx_buffer[0]) {
            case OS_IO_PACKET_TYPE_SE_EVT:
            case OS_IO_PACKET_TYPE_SEPH:
                memcpy(G_io_seproxyhal_spi_buffer, &G_io_rx_buffer[1], status - 1);
                io_event(CHANNEL_APDU);
                break;

            case OS_IO_PACKET_TYPE_RAW_APDU:
            case OS_IO_PACKET_TYPE_USB_HID_APDU:
            case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
            case OS_IO_PACKET_TYPE_USB_U2F_HID_APDU:
            case OS_IO_PACKET_TYPE_BLE_APDU:
            case OS_IO_PACKET_TYPE_NFC_APDU:
                os_io_tx_cmd(G_io_rx_buffer[0], err_buffer, sizeof(err_buffer), 0);
                break;

            default:
                break;
        }
    }
}

#ifdef HAVE_BAGL
void io_seproxyhal_display_default(const bagl_element_t *element)
{
    io_seph_ux_display_bagl_element(element);
}
#endif  // HAVE_BAGL

unsigned int io_seph_is_status_sent(void)
{
    return 1;
}

unsigned short io_exchange(unsigned char channel_and_flags, unsigned short tx_len)
{
    int status = 0;

    if ((channel_and_flags & ~(IO_FLAGS)) != CHANNEL_APDU) {
        return 0;
    }

    if (tx_len && !(channel_and_flags & IO_ASYNCH_REPLY)) {
        memmove(G_io_tx_buffer, G_io_apdu_buffer, tx_len);
        status                 = io_legacy_apdu_tx(G_io_tx_buffer, tx_len);
        G_io_app.apdu_media    = IO_APDU_MEDIA_NONE;
        io_os_legacy_apdu_type = APDU_TYPE_NONE;
        if (channel_and_flags & IO_RETURN_AFTER_TX) {
            return 0;
        }
    }

    if (!(channel_and_flags & IO_ASYNCH_REPLY)) {
        G_io_app.apdu_media    = IO_APDU_MEDIA_NONE;
        io_os_legacy_apdu_type = APDU_TYPE_NONE;
    }

    G_io_app.apdu_length = 0;

    status = 0;
    while (status <= 0) {
        status = io_legacy_apdu_rx();
    }
    G_io_app.apdu_length = status;

    return status;
}

void io_seproxyhal_init(void)
{
    os_io_init_t init_io;

    io_os_legacy_apdu_type = APDU_TYPE_NONE;
    init_io.syscall        = G_io_syscall_flag;
    init_io.usb.pid        = 0;
    init_io.usb.vid        = 0;
    init_io.usb.class_mask = 0;
    memset(init_io.usb.name, 0, sizeof(init_io.usb.name));
#ifdef HAVE_IO_USB
    init_io.usb.class_mask = USBD_LEDGER_CLASS_HID | USBD_LEDGER_CLASS_WEBUSB;
#ifdef HAVE_IO_U2F
    init_io.usb.class_mask |= USBD_LEDGER_CLASS_HID_U2F;

    init_io.usb.hid_u2f_settings.protocol_version            = 2;
    init_io.usb.hid_u2f_settings.major_device_version_number = 0;
    init_io.usb.hid_u2f_settings.minor_device_version_number = 1;
    init_io.usb.hid_u2f_settings.build_device_version_number = 0;
    init_io.usb.hid_u2f_settings.capabilities_flag           = 0;
#endif  // HAVE_IO_U2F
#endif  // !HAVE_IO_USB

    init_io.ble.profile_mask = 0;
#ifdef HAVE_BLE
    init_io.ble.profile_mask = BLE_LEDGER_PROFILE_APDU;
#endif  // !HAVE_BLE

    os_io_init(&init_io);
    os_io_start();
    os_io_seph_cmd_general_status();
}

void USB_power(unsigned char enabled)
{
    UNUSED(enabled);
}

#ifdef HAVE_BLE
void BLE_power(unsigned char powered, const char *discovered_name)
{
    UNUSED(powered);
    UNUSED(discovered_name);
}
#endif  // HAVE_BLE

int io_legacy_apdu_rx(void)
{
    int                      status      = 0;
    os_io_apdu_post_action_t post_action = OS_IO_APDU_POST_ACTION_NONE;

    status = os_io_rx_evt(G_io_rx_buffer, sizeof(G_io_rx_buffer), NULL);

    if (status > 0) {
        switch (G_io_rx_buffer[0]) {
            case OS_IO_PACKET_TYPE_SE_EVT:
            case OS_IO_PACKET_TYPE_SEPH:
                memcpy(G_io_seproxyhal_spi_buffer, &G_io_rx_buffer[1], status - 1);
                if (G_io_rx_buffer[1] == SEPROXYHAL_TAG_ITC_EVENT) {
                    status = io_process_itc_ux_event(G_io_seproxyhal_spi_buffer, status - 1);
                }
                else {
                    io_event(CHANNEL_APDU);
                    status = 0;
                }
                break;

            case OS_IO_PACKET_TYPE_RAW_APDU:
            case OS_IO_PACKET_TYPE_USB_HID_APDU:
            case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
            case OS_IO_PACKET_TYPE_USB_U2F_HID_APDU:
            case OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR:
            case OS_IO_PACKET_TYPE_BLE_APDU:
            case OS_IO_PACKET_TYPE_NFC_APDU:
                io_os_legacy_apdu_type = G_io_rx_buffer[0];
                if (G_io_rx_buffer[APDU_OFF_CLA + 1] == DEFAULT_APDU_CLA) {
                    size_t      buffer_out_length = sizeof(G_io_rx_buffer);
                    bolos_err_t err               = os_io_handle_default_apdu(&G_io_rx_buffer[1],
                                                                status - 1,
                                                                G_io_tx_buffer,
                                                                &buffer_out_length,
                                                                &post_action);
                    if (err != SWO_SUCCESS) {
                        buffer_out_length = 0;
                    }
                    G_io_tx_buffer[buffer_out_length++] = err >> 8;
                    G_io_tx_buffer[buffer_out_length++] = err;
                    status                              = os_io_tx_cmd(
                        io_os_legacy_apdu_type, G_io_tx_buffer, buffer_out_length, 0);
                    io_os_legacy_apdu_type = APDU_TYPE_NONE;
                    if (post_action == OS_IO_APDU_POST_ACTION_EXIT) {
                        os_sched_exit(-1);
                    }
                    if (status > 0) {
                        status = 0;
                    }
                }
                else {
                    G_io_app.apdu_media = get_media_from_apdu_type(io_os_legacy_apdu_type);
                    status -= 1;
                    memmove(G_io_apdu_buffer, &G_io_rx_buffer[1], status);
#ifdef HAVE_IO_U2F
                    if (io_os_legacy_apdu_type == OS_IO_PACKET_TYPE_USB_U2F_HID_APDU) {
                        G_io_u2f.media      = U2F_MEDIA_USB;
                        G_io_app.apdu_state = APDU_U2F;
                    }
                    else if (io_os_legacy_apdu_type == OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR) {
                        G_io_u2f.media      = U2F_MEDIA_USB;
                        G_io_app.apdu_state = APDU_IDLE;
                    }
                    else if (io_os_legacy_apdu_type == OS_IO_PACKET_TYPE_NFC_APDU) {
                        G_io_u2f.media      = U2F_MEDIA_NFC;
                        G_io_app.apdu_state = APDU_U2F;
                    }
#endif  // HAVE_IO_U2F
                }
                break;

            default:
                status = 0;
                break;
        }
    }

    return status;
}

int io_legacy_apdu_tx(const unsigned char *buffer, unsigned short length)
{
    int status             = os_io_tx_cmd(io_os_legacy_apdu_type, buffer, length, 0);
    G_io_app.apdu_media    = IO_APDU_MEDIA_NONE;
    io_os_legacy_apdu_type = APDU_TYPE_NONE;
#ifdef HAVE_IO_U2F
    G_io_u2f.media = U2F_MEDIA_NONE;
#endif  // HAVE_IO_U2F

    return status;
}
