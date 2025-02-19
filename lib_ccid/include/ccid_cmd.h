/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "ccid_types.h"

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
int32_t CCID_CMD_process(ccid_device_t *handle);
uint8_t CCID_CMD_abort(ccid_device_t *handle, uint8_t slot, uint8_t seq);

// Legacy
void io_usb_ccid_configure_pinpad(uint8_t enabled);
void io_usb_ccid_set_card_inserted(uint32_t inserted);
