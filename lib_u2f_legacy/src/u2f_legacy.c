/* @BANNER@ */

#ifdef HAVE_IO_U2F

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "u2f_impl.h"
#include "u2f_processing.h"
#include "u2f_service.h"
#include "os_io.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
u2f_service_t G_io_u2f;

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void u2f_message_set_autoreply_wait_user_presence(u2f_service_t *service, bool enabled)
{
    (void) service;
    (void) enabled;
    uint8_t buffer[2] = {0xFF, 0xFF};
    os_io_tx_cmd(OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, buffer, sizeof(buffer), 0);
}

void u2f_message_reply(u2f_service_t *service, uint8_t cmd, uint8_t *buffer, uint16_t len)
{
    (void) service;
    if (cmd == U2F_COMMAND_HID_CBOR) {
        os_io_tx_cmd(OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR, buffer, len, 0);
    }
    else {
        os_io_tx_cmd(cmd, buffer, len, 0);
    }
}

void u2f_transport_ctap2_send_keepalive(u2f_service_t *service, uint8_t reason)
{
    (void) service;
    (void) reason;
    uint8_t buffer[2];

    buffer[0] = U2F_COMMAND_HID_KEEP_ALIVE;
    buffer[1] = reason;

    os_io_tx_cmd(OS_IO_PACKET_TYPE_USB_U2F_HID_RAW, buffer, sizeof(buffer), 0);
}

#endif  // HAVE_IO_U2F
