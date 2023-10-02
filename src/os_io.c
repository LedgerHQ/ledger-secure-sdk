
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "bolos.h"
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
unsigned char G_io_seph_rx_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B + 1];
uint16_t      G_io_seph_rx_buffer_length;
#ifdef HAVE_BOLOS
io_seph_app_t G_io_app;
#endif  // HAVE_BOLOS

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int32_t os_io_rx_evt(uint32_t *timeout)
{
    int32_t  status = 0;
    uint16_t length = 0;

    if (!G_io_seph_rx_buffer_length) {
        status = io_seph_se_rx_event(
            G_io_seph_rx_buffer, sizeof(G_io_seph_rx_buffer), (unsigned int *) timeout, true, 0);

        if (status == -1) {
            status = io_seph_cmd_general_status();
            if (status < 0) {
                return status;
            }
            status = io_seph_se_rx_event(G_io_seph_rx_buffer,
                                         sizeof(G_io_seph_rx_buffer),
                                         (unsigned int *) timeout,
                                         true,
                                         0);
        }
        if (status < 0) {
            return status;
        }
        length = (uint16_t) status;
    }
    else {
        length                     = G_io_seph_rx_buffer_length;
        G_io_seph_rx_buffer_length = 0;
    }

    switch (G_io_seph_rx_buffer[1]) {
#ifdef HAVE_IO_USB
        case SEPROXYHAL_TAG_USB_EVENT:
        case SEPROXYHAL_TAG_USB_EP_XFER_EVENT:
            status = USBD_LEDGER_rx_seph_evt(
                G_io_seph_rx_buffer, length, G_io_apdu_rx_buffer, sizeof(G_io_apdu_rx_buffer));
            break;
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
        case SEPROXYHAL_TAG_BLE_RECV_EVENT:
            status = BLE_LEDGER_rx_seph_evt(
                G_io_seph_rx_buffer, length, G_io_apdu_rx_buffer, sizeof(G_io_apdu_rx_buffer));
            break;
#endif  // HAVE_BLE

#ifdef HAVE_NFC
        case SEPROXYHAL_TAG_NFC_APDU_EVENT:
            if (length >= sizeof(G_io_apdu_rx_buffer) - 1) {
                length = sizeof(G_io_apdu_rx_buffer) - 1;
            }
            G_io_apdu_rx_buffer[0] = OS_IO_PACKET_TYPE_NFC_APDU;
            G_io_seph_rx_buffer[0] = G_io_apdu_rx_buffer[0];
            memcpy(&G_io_apdu_rx_buffer[1], &G_io_seph_rx_buffer[4], length);
            break;
#endif  // HAVE_NFC

        case SEPROXYHAL_TAG_CAPDU_EVENT:
            if (length >= sizeof(G_io_apdu_rx_buffer) - 1) {
                length = sizeof(G_io_apdu_rx_buffer) - 1;
            }
            G_io_apdu_rx_buffer[0] = OS_IO_PACKET_TYPE_RAW_APDU;
            G_io_seph_rx_buffer[0] = G_io_apdu_rx_buffer[0];
            memcpy(&G_io_apdu_rx_buffer[1], &G_io_seph_rx_buffer[4], length);
            status = length - 3;
            break;

        default:
            break;
    }

    return status;
}

int os_io_tx_cmd(const unsigned char *buffer PLENGTH(length),
                 unsigned short              length,
                 unsigned int               *timeout_ms)
{
    int status = io_seph_tx(buffer, length, (unsigned int *) timeout_ms);

    if (status == -1) {
        status = io_seph_se_rx_event(G_io_seph_rx_buffer,
                                     sizeof(G_io_seph_rx_buffer),
                                     (unsigned int *) timeout_ms,
                                     false,
                                     0);
        if (status >= 0) {
            G_io_seph_rx_buffer_length = status;
            status                     = io_seph_tx(buffer, length, NULL);  // TODO : process the rx packet?
        }
    }
    return status;
}

int32_t os_io_init(void)
{
    memset(&G_io_app, 0, sizeof(G_io_app));
    G_io_app.apdu_media = IO_APDU_MEDIA_NONE;

#ifdef HAVE_BAGL
    io_seph_ux_init_button();
#endif

#if !defined(HAVE_BOLOS)
    check_api_level(CX_COMPAT_APILEVEL);
#endif  // !HAVE_BOLOS

#if (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))
    io_seph_cmd_mcu_protect();
#endif  // (!HAVE_BOLOS && HAVE_MCU_PROTECT)

#if !defined(HAVE_BOLOS) && defined(HAVE_PENDING_REVIEW_SCREEN)
    check_audited_app();
#endif  // !HAVE_BOLOS && HAVE_PENDING_REVIEW_SCREEN

#ifdef HAVE_IO_USB
    USBD_LEDGER_init();
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
    BLE_LEDGER_init();
#endif  // HAVE_BLE

#ifdef HAVE_NFC
    // TODO
#endif  // HAVE_NFC

#if !defined(HAVE_BOLOS) && defined(HAVE_PENDING_REVIEW_SCREEN)
    // check_audited_app(); // TODO
#endif  // !defined(HAVE_BOLOS) && defined(HAVE_PENDING_REVIEW_SCREEN)

    return 0;
}

int32_t os_io_start(os_io_init_t *init)
{
    if (!init) {
        return -1;
    }

#ifdef HAVE_IO_USB
    USBD_LEDGER_start(init->usb.pid, init->usb.vid, init->usb.name, init->usb.class_mask);
#endif  // HAVE_IO_USB

#ifdef HAVE_BLE
    BLE_LEDGER_start(init->ble.profile_mask);
#endif  // HAVE_BLE

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
