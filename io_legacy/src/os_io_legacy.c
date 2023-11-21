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
}

void io_seproxyhal_disable_io(void)  // TODO_IO
{
    // os_io_seph_cmd_usb_disconnect();
}

void io_seproxyhal_io_heartbeat(void)
{
    // TODO_IO
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

__attribute((weak)) unsigned char io_event(unsigned char channel)
{
    UNUSED(channel);

    if ((G_io_seproxyhal_spi_buffer[0] == SEPROXYHAL_TAG_TICKER_EVENT)
        || (G_io_seproxyhal_spi_buffer[0] == SEPROXYHAL_TAG_BUTTON_PUSH_EVENT)
        || (G_io_seproxyhal_spi_buffer[0] == SEPROXYHAL_TAG_STATUS_EVENT)
        || (G_io_seproxyhal_spi_buffer[0] == SEPROXYHAL_TAG_FINGER_EVENT)
        || (G_io_seproxyhal_spi_buffer[0] == SEPROXYHAL_TAG_POWER_BUTTON_EVENT)) {
        G_ux_params.ux_id = BOLOS_UX_EVENT;
        G_ux_params.len   = 0;
        os_ux(&G_ux_params);
    }

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
        status                 = os_io_tx_cmd(io_os_legacy_apdu_type, G_io_tx_buffer, tx_len, 0);
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
    while (!status) {
        status = os_io_rx_evt(G_io_rx_buffer, sizeof(G_io_rx_buffer), NULL);
        if (status > 0) {
            switch (G_io_rx_buffer[0]) {
                case OS_IO_PACKET_TYPE_SE_EVT:
                case OS_IO_PACKET_TYPE_SEPH:
                    if (G_io_rx_buffer[1] == SEPROXYHAL_TAG_ITC_EVENT) {
                        io_process_itc_ux_event(&G_io_rx_buffer[1], status - 1);
                    }
                    else {
                        memcpy(G_io_seproxyhal_spi_buffer, &G_io_rx_buffer[1], status - 1);
                        io_event(CHANNEL_APDU);
                    }
                    status = 0;
                    break;

                case OS_IO_PACKET_TYPE_RAW_APDU:
                case OS_IO_PACKET_TYPE_USB_HID_APDU:
                case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
                case OS_IO_PACKET_TYPE_BLE_APDU:
                    io_os_legacy_apdu_type = G_io_rx_buffer[0];
                    if (G_io_rx_buffer[APDU_OFF_CLA + 1] == DEFAULT_APDU_CLA) {
                        size_t buffer_out_length = sizeof(G_io_rx_buffer);
                        os_io_handle_default_apdu(
                            &G_io_rx_buffer[1], status - 1, G_io_tx_buffer, &buffer_out_length);
                        os_io_tx_cmd(io_os_legacy_apdu_type, G_io_tx_buffer, buffer_out_length, 0);
                        io_os_legacy_apdu_type = APDU_TYPE_NONE;
                        status                 = 0;
                    }
                    else {
                        G_io_app.apdu_media  = get_media_from_apdu_type(io_os_legacy_apdu_type);
                        G_io_app.apdu_length = status - 1;
                        memmove(G_io_rx_buffer, &G_io_rx_buffer[1], G_io_app.apdu_length);
                    }
                    break;

#ifdef HAVE_IO_U2F
                case OS_IO_PACKET_TYPE_USB_U2F_HID_APDU:
                    io_os_legacy_apdu_type = G_io_rx_buffer[0];
                    G_io_app.apdu_media    = get_media_from_apdu_type(io_os_legacy_apdu_type);
                    G_io_u2f.media         = U2F_MEDIA_USB;
                    G_io_app.apdu_length   = status - 1;
                    memmove(G_io_rx_buffer, &G_io_rx_buffer[1], G_io_app.apdu_length);
                    break;
#endif // HAVE_IO_U2F

                default:
                    status = 0;
                    break;
            }
        }
        else if (status < 0) {
            status = -1;
        }
    }

    return G_io_app.apdu_length;
}

void io_seproxyhal_init(void)
{
    os_io_init_t init_io;

    io_os_legacy_apdu_type = APDU_TYPE_NONE;
    init_io.syscall        = G_io_syscall_flag;
    init_io.usb.pid        = 0;
    init_io.usb.vid        = 0;
    init_io.usb.name       = 0;
    init_io.usb.class_mask = 0;
#ifdef HAVE_IO_USB
    init_io.usb.class_mask |= USBD_LEDGER_CLASS_HID;
#ifdef HAVE_WEBUSB
    init_io.usb.class_mask |= USBD_LEDGER_CLASS_WEBUSB;
#endif  // HAVE_WEBUSB
#ifdef HAVE_IO_U2F
    init_io.usb.class_mask |= USBD_LEDGER_CLASS_HID_U2F;

    init_io.u2f_settings.protocol_version            = 2;
    init_io.u2f_settings.major_device_version_number = 0;
    init_io.u2f_settings.minor_device_version_number = 1;
    init_io.u2f_settings.build_device_version_number = 0;
    init_io.u2f_settings.capabilities_flag           = 0;
#endif  // HAVE_IO_U2F
#endif  // !HAVE_IO_USB

    init_io.ble.profile_mask = 0;
#ifdef HAVE_BLE
    init_io.ble.profile_mask = BLE_LEDGER_PROFILE_APDU;
#ifdef HAVE_IO_U2F
#endif  // HAVE_IO_U2F
#endif  // !HAVE_BLE

    os_io_start(&init_io);
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

void io_seproxyhal_request_mcu_status(void) {}