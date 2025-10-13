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
#include <stdio.h>
#include "errors.h"
#include "os_helpers.h"
#include "os_pin.h"
#include "os_io.h"
#include "os_apdu.h"
#include "os_io_default_apdu.h"
#include "os_io_legacy.h"
#include "os.h"

#ifdef HAVE_IO_USB
#include "usbd_ledger.h"
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
#include "ble_ledger.h"
#endif  // HAVE_BLE

#ifdef HAVE_IO_U2F
#include "u2f_service.h"
#endif  // HAVE_IO_U2F

#ifdef HAVE_NFC_READER
#include "nfc_ledger.h"
#endif  // HAVE_NFC_READER

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
#ifdef HAVE_NFC_READER
static void io_nfc_event(uint8_t *buffer_in, size_t buffer_in_length);
static void io_nfc_ticker(void);
static void io_nfc_process_events(void);
#endif

/* Exported variables --------------------------------------------------------*/
io_seph_app_t G_io_app;
unsigned char G_io_asynch_ux_callback;

#ifdef HAVE_IO_U2F
extern u2f_service_t G_io_u2f;
#endif  // HAVE_IO_U2F

#ifdef HAVE_NFC_READER
struct nfc_reader_context G_io_reader_ctx;
#endif  // HAVE_NFC_READER

#ifdef HAVE_BOLOS_APP_STACK_CANARY
extern unsigned int app_stack_canary;
#endif  // HAVE_BOLOS_APP_STACK_CANARY

/* Private variables ---------------------------------------------------------*/

// Variable containing the type of the APDU response to send back.
static apdu_type_t io_os_legacy_apdu_type;
static uint8_t     need_to_start_io;

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
    os_reset();
}

void io_seproxyhal_disable_io(void)
{
    os_io_stop();
}

void io_seproxyhal_io_heartbeat(void)
{
    uint16_t      err = 0x6601;
    unsigned char err_buffer[2];
    int           status = os_io_rx_evt(G_io_rx_buffer, sizeof(G_io_rx_buffer), NULL, true);

    if (os_perso_is_pin_set() == BOLOS_TRUE && os_global_pin_is_validated() != BOLOS_TRUE) {
        err = SWO_SEC_PIN_15;
    }

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
            case OS_IO_PACKET_TYPE_USB_CCID_APDU:
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

void io_seproxyhal_request_mcu_status(void) {}

#ifdef HAVE_BAGL
void io_seproxyhal_display_default(const bagl_element_t *element)
{
    io_seph_ux_display_bagl_element(element);
}
#endif  // HAVE_BAGL

void io_seph_send(const unsigned char *buffer, unsigned short length)
{
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, length + 1, 0);
}

unsigned int io_seph_is_status_sent(void)
{
    return 1;
}

unsigned short io_seph_recv(unsigned char *buffer, unsigned short maxlength, unsigned int flags)
{
    UNUSED(maxlength);
    UNUSED(flags);
    int status = os_io_rx_evt(G_io_rx_buffer, sizeof(G_io_rx_buffer), NULL, true);

    if (status > 0) {
        switch (G_io_rx_buffer[0]) {
            case OS_IO_PACKET_TYPE_SE_EVT:
            case OS_IO_PACKET_TYPE_SEPH:
                status -= 1;
                memcpy(buffer, &G_io_rx_buffer[1], status);
                if (G_io_rx_buffer[1] == SEPROXYHAL_TAG_ITC_EVENT) {
                    status = io_process_itc_ux_event(buffer, status);
                }
                else {
                    io_event(CHANNEL_APDU);
                    status = 0;
                }
                break;
            default:
                break;
        }
    }

    return status;
}

unsigned short io_exchange(unsigned char channel_and_flags, unsigned short tx_len)
{
    int status = 0;

    if (need_to_start_io) {
        os_io_start();
        need_to_start_io = 0;
    }

    if ((channel_and_flags & ~(IO_FLAGS)) != CHANNEL_APDU) {
        return 0;
    }

    if (tx_len && !(channel_and_flags & IO_ASYNCH_REPLY)) {
        memmove(G_io_tx_buffer, G_io_apdu_buffer, tx_len);
        io_legacy_apdu_tx(G_io_tx_buffer, tx_len);
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
        status = io_legacy_apdu_rx(1);
    }
    G_io_app.apdu_length = status;
    PRINTF("io_exchange end");

    return status;
}

void io_seproxyhal_init(void)
{
    os_io_init_t init_io;

    io_os_legacy_apdu_type = APDU_TYPE_NONE;
    init_io.usb.pid        = 0;
#ifdef HAVE_LEGACY_PID
#if defined(TARGET_NANOX)
    init_io.usb.pid = USBD_LEDGER_INVERTED_PRODUCT_NANOX;
#endif  // TARGET_NANOX
#if defined(TARGET_NANOS2)
    init_io.usb.pid = USBD_LEDGER_INVERTED_PRODUCT_NANOS_PLUS;
#endif  // TARGET_NANOS2
#if defined(TARGET_FATSTACKS) || defined(TARGET_STAX)
    init_io.usb.pid = USBD_LEDGER_INVERTED_PRODUCT_STAX;
#endif  // TARGET_FATSTACKS || TARGET_STAX
#if defined(TARGET_FLEX)
    init_io.usb.pid = USBD_LEDGER_INVERTED_PRODUCT_FLEX;
#endif  // TARGET_FLEX
#if defined(TARGET_APEX_P)
    init_io.usb.pid = USBD_LEDGER_INVERTED_PRODUCT_APEX_P;
#endif  // TARGET_APEX_M
#if defined(TARGET_FLEX)
    init_io.usb.pid = USBD_LEDGER_INVERTED_PRODUCT_APEX_M;
#endif  // TARGET_APEX_M
#endif  // HAVE_LEGACY_PID
    init_io.usb.vid        = 0;
    init_io.usb.class_mask = 0;
    memset(init_io.usb.name, 0, sizeof(init_io.usb.name));
#ifdef HAVE_IO_USB
    init_io.usb.class_mask = USBD_LEDGER_CLASS_HID;
#ifdef HAVE_WEBUSB
    init_io.usb.class_mask |= USBD_LEDGER_CLASS_WEBUSB;
#endif  // HAVE_WEBUSB
#ifdef HAVE_IO_U2F
    init_io.usb.class_mask |= USBD_LEDGER_CLASS_HID_U2F;

    init_io.usb.hid_u2f_settings.protocol_version            = 2;
    init_io.usb.hid_u2f_settings.major_device_version_number = 0;
    init_io.usb.hid_u2f_settings.minor_device_version_number = 1;
    init_io.usb.hid_u2f_settings.build_device_version_number = 0;
#ifdef HAVE_FIDO2
    init_io.usb.hid_u2f_settings.capabilities_flag = U2F_HID_CAPABILITY_CBOR;
#else   // !HAVE_FIDO2
    init_io.usb.hid_u2f_settings.capabilities_flag = 0;
#endif  // !HAVE_FIDO2
#endif  // HAVE_IO_U2F
#ifdef HAVE_CCID_USB
    init_io.usb.class_mask |= USBD_LEDGER_CLASS_CCID_BULK;
#endif  // HAVE_CCID_USB
#ifdef HAVE_USB_HIDKBD
    init_io.usb.class_mask |= USBD_LEDGER_CLASS_HID_KBD;
#endif  // HAVE_USB_HIDKBD
#endif  // !HAVE_IO_USB

    init_io.ble.profile_mask = 0;
#ifdef HAVE_BLE
    init_io.ble.profile_mask = BLE_LEDGER_PROFILE_APDU;
#endif  // !HAVE_BLE

#ifdef HAVE_NFC_READER
    memset((void *) &G_io_reader_ctx, 0, sizeof(G_io_reader_ctx));
    G_io_reader_ctx.apdu_rx          = G_io_seproxyhal_spi_buffer;
    G_io_reader_ctx.apdu_rx_max_size = sizeof(G_io_seproxyhal_spi_buffer);
#endif  // HAVE_NFC_READER

#ifdef HAVE_BOLOS_APP_STACK_CANARY
    app_stack_canary = APP_STACK_CANARY_MAGIC;
#endif  // HAVE_BOLOS_APP_STACK_CANARY

    os_io_init(&init_io);
    need_to_start_io = 1;
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

unsigned int os_io_seph_recv_and_process(unsigned int dont_process_ux_events)
{
    int status = io_legacy_apdu_rx(!dont_process_ux_events);

    if (status != 0) {
        status = 1;
    }

    return status;
}

int io_legacy_apdu_rx(uint8_t handle_ux_events)
{
    int                      status      = 0;
    os_io_apdu_post_action_t post_action = OS_IO_APDU_POST_ACTION_NONE;

    status = os_io_rx_evt(G_io_rx_buffer, sizeof(G_io_rx_buffer), NULL, true);

    if (status > 0) {
        switch (G_io_rx_buffer[0]) {
            case OS_IO_PACKET_TYPE_SE_EVT:
            case OS_IO_PACKET_TYPE_SEPH:
                memcpy(G_io_seproxyhal_spi_buffer, &G_io_rx_buffer[1], status - 1);
                if (G_io_rx_buffer[1] == SEPROXYHAL_TAG_ITC_EVENT) {
                    status = io_process_itc_ux_event(G_io_seproxyhal_spi_buffer, status - 1);
                }
                else {
                    status = 0;
                    if (!handle_ux_events) {
                        if ((G_io_seproxyhal_spi_buffer[0] != SEPROXYHAL_TAG_FINGER_EVENT)
                            && (G_io_seproxyhal_spi_buffer[0] != SEPROXYHAL_TAG_BUTTON_PUSH_EVENT)
                            && (G_io_seproxyhal_spi_buffer[0] != SEPROXYHAL_TAG_TICKER_EVENT)
                            && (G_io_seproxyhal_spi_buffer[0] != SEPROXYHAL_TAG_STATUS_EVENT)) {
                            io_event(CHANNEL_APDU);
                        }
                    }
                    else {
                        io_event(CHANNEL_APDU);
                    }
                }
                break;

            case OS_IO_PACKET_TYPE_RAW_APDU:
            case OS_IO_PACKET_TYPE_USB_HID_APDU:
            case OS_IO_PACKET_TYPE_USB_CCID_APDU:
            case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
            case OS_IO_PACKET_TYPE_USB_U2F_HID_APDU:
            case OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR:
            case OS_IO_PACKET_TYPE_USB_U2F_HID_CANCEL:
            case OS_IO_PACKET_TYPE_USB_U2F_HID_RAW:
            case OS_IO_PACKET_TYPE_BLE_APDU:
            case OS_IO_PACKET_TYPE_NFC_APDU:
                io_os_legacy_apdu_type = G_io_rx_buffer[0];
                if (os_perso_is_pin_set() == BOLOS_TRUE
                    && os_global_pin_is_validated() != BOLOS_TRUE) {
                    bolos_err_t err   = SWO_SEC_PIN_15;
                    G_io_tx_buffer[0] = err >> 8;
                    G_io_tx_buffer[1] = err;
                    status            = os_io_tx_cmd(io_os_legacy_apdu_type, G_io_tx_buffer, 2, 0);
                }
#ifndef HAVE_BOLOS_NO_DEFAULT_APDU
                else if (G_io_rx_buffer[APDU_OFF_CLA + 1] == DEFAULT_APDU_CLA) {
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
#ifndef USE_OS_IO_STACK
                        os_io_stop();
#endif  // USE_OS_IO_STACK
                        os_sched_exit(-1);
                    }
                    if (status > 0) {
                        status = 0;
                    }
                }
#endif  // HAVE_BOLOS_NO_DEFAULT_APDU
                else {
                    G_io_app.apdu_media = get_media_from_apdu_type(io_os_legacy_apdu_type);
                    status -= 1;
                    memmove(G_io_apdu_buffer, &G_io_rx_buffer[1], status);
#ifdef HAVE_IO_U2F
                    if (io_os_legacy_apdu_type == APDU_TYPE_USB_HID) {
                        G_io_u2f.media      = U2F_MEDIA_USB;
                        G_io_app.apdu_state = APDU_USB_HID;
                    }
                    else if (io_os_legacy_apdu_type == APDU_TYPE_USB_U2F) {
                        G_io_u2f.media      = U2F_MEDIA_USB;
                        G_io_app.apdu_state = APDU_U2F;
                    }
                    else if (io_os_legacy_apdu_type == APDU_TYPE_USB_U2F_CBOR) {
                        G_io_u2f.media      = U2F_MEDIA_USB;
                        G_io_app.apdu_state = APDU_U2F_CBOR;
                    }
                    else if (io_os_legacy_apdu_type == APDU_TYPE_USB_U2F_CANCEL) {
                        G_io_u2f.media      = U2F_MEDIA_USB;
                        G_io_app.apdu_state = APDU_U2F_CANCEL;
                        status += 1;
                    }
                    else if (io_os_legacy_apdu_type == APDU_TYPE_NFC) {
                        G_io_u2f.media      = U2F_MEDIA_NFC;
                        G_io_app.apdu_state = APDU_U2F;
                    }
#endif  // HAVE_IO_U2F
                }
                break;
            case OS_IO_PACKET_TYPE_AT_CMD:
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
    int status = os_io_tx_cmd(io_os_legacy_apdu_type, buffer, length, 0);

    G_io_app.apdu_media    = IO_APDU_MEDIA_NONE;
    io_os_legacy_apdu_type = APDU_TYPE_NONE;
#ifdef HAVE_IO_U2F
    G_io_u2f.media = U2F_MEDIA_NONE;
#endif  // HAVE_IO_U2F

    return status;
}

#ifdef HAVE_NFC_READER

void io_nfc_event(uint8_t *buffer_in, size_t buffer_in_length)
{
    if (buffer_in_length < 3) {
        return;
    }
    size_t size = U2BE(buffer_in, 1);
    if (buffer_in_length < 3 + size) {
        return;
    }

    if (size >= 1) {
        switch (buffer_in[3]) {
            case SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED: {
                G_io_reader_ctx.event_happened = true;
                G_io_reader_ctx.last_event     = CARD_DETECTED;
                G_io_reader_ctx.card.tech
                    = (buffer_in[4] == SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED_A) ? NFC_A : NFC_B;
                G_io_reader_ctx.card.nfcid_len = MIN(size - 2, sizeof(G_io_reader_ctx.card.nfcid));
                memcpy((void *) G_io_reader_ctx.card.nfcid,
                       G_io_seproxyhal_spi_buffer + 5,
                       G_io_reader_ctx.card.nfcid_len);
            } break;

            case SEPROXYHAL_TAG_NFC_EVENT_CARD_LOST:
                if (G_io_reader_ctx.evt_callback != NULL) {
                    G_io_reader_ctx.event_happened = true;
                    G_io_reader_ctx.last_event     = CARD_REMOVED;
                }
                break;
        }
    }
}

void os_io_nfc_reader_rx(uint8_t *in_buffer, size_t in_buffer_len)
{
    if (in_buffer_len <= G_io_reader_ctx.apdu_rx_max_size) {
        memcpy(G_io_reader_ctx.apdu_rx, in_buffer, in_buffer_len);
        G_io_reader_ctx.response_received = true;
        G_io_reader_ctx.apdu_rx_len       = in_buffer_len;
        io_nfc_process_events();
    }
}

void io_nfc_process_events(void)
{
    if (G_io_reader_ctx.response_received) {
        G_io_reader_ctx.response_received = false;
        if (G_io_reader_ctx.resp_callback != NULL) {
            nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
            G_io_reader_ctx.resp_callback = NULL;
            resp_cb(false, false, G_io_reader_ctx.apdu_rx, G_io_reader_ctx.apdu_rx_len);
        }
    }

    if (G_io_reader_ctx.resp_callback != NULL && G_io_reader_ctx.remaining_ms == 0) {
        nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
        G_io_reader_ctx.resp_callback = NULL;
        resp_cb(false, true, NULL, 0);
    }

    if (G_io_reader_ctx.event_happened) {
        G_io_reader_ctx.event_happened = 0;

        // If card is removed during an APDU processing, call the resp_callback with an error
        if (G_io_reader_ctx.resp_callback != NULL && G_io_reader_ctx.last_event == CARD_REMOVED) {
            nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
            G_io_reader_ctx.resp_callback = NULL;
            resp_cb(true, false, NULL, 0);
        }

        if (G_io_reader_ctx.evt_callback != NULL) {
            G_io_reader_ctx.evt_callback(G_io_reader_ctx.last_event,
                                         (struct card_info *) &G_io_reader_ctx.card);
        }
        if (G_io_reader_ctx.last_event == CARD_REMOVED) {
            memset((void *) &G_io_reader_ctx.card, 0, sizeof(G_io_reader_ctx.card));
        }
    }
}

void io_nfc_ticker(void)
{
    if (G_io_reader_ctx.resp_callback != NULL) {
        if (G_io_reader_ctx.remaining_ms <= 100) {
            G_io_reader_ctx.remaining_ms = 0;
        }
        else {
            G_io_reader_ctx.remaining_ms -= 100;
        }
    }
}

void os_io_nfc_evt(uint8_t *buffer_in, size_t buffer_in_length)
{
    switch (buffer_in[0]) {
        case SEPROXYHAL_TAG_NFC_EVENT:
            io_nfc_event(buffer_in, buffer_in_length);
            io_nfc_process_events();
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            io_nfc_ticker();
            io_nfc_process_events();
            break;

        default:
            break;
    }
}

bool io_nfc_reader_send(const uint8_t      *cmd_data,
                        size_t              cmd_len,
                        nfc_resp_callback_t callback,
                        int                 timeout_ms)
{
    G_io_reader_ctx.resp_callback = PIC(callback);
    os_io_tx_cmd(APDU_TYPE_NFC, PIC(cmd_data), cmd_len, 0);

    G_io_reader_ctx.response_received = false;
    G_io_reader_ctx.remaining_ms      = timeout_ms;

    return true;
}

bool io_nfc_reader_start(nfc_evt_callback_t callback)
{
    G_io_reader_ctx.evt_callback      = PIC(callback);
    G_io_reader_ctx.reader_mode       = true;
    G_io_reader_ctx.event_happened    = false;
    G_io_reader_ctx.resp_callback     = NULL;
    G_io_reader_ctx.response_received = false;
    os_io_nfc_cmd_start_reader();
    return true;
}

void io_nfc_reader_stop(void)
{
    G_io_reader_ctx.evt_callback      = NULL;
    G_io_reader_ctx.reader_mode       = false;
    G_io_reader_ctx.event_happened    = false;
    G_io_reader_ctx.resp_callback     = NULL;
    G_io_reader_ctx.response_received = false;
    os_io_nfc_cmd_stop();
}

bool io_nfc_is_reader(void)
{
    return G_io_reader_ctx.reader_mode;
}
#endif  // HAVE_NFC_READER
