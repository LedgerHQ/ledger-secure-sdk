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
    UNUSED(service);
    UNUSED(enabled);
    uint8_t buffer[2] = {0x69, 0x85};
    os_io_tx_cmd(OS_IO_PACKET_TYPE_USB_U2F_HID_APDU, buffer, sizeof(buffer), 0);
}

#endif  // HAVE_IO_U2F