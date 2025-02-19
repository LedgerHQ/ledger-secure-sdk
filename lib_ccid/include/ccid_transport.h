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
void CCID_TRANSPORT_init(ccid_transport_t *handle);
void CCID_TRANSPORT_rx(ccid_transport_t *handle, uint8_t *buffer, uint16_t length);
void CCID_TRANSPORT_tx(ccid_transport_t *handle, const uint8_t *buffer, uint16_t length);
void CCID_TRANSPORT_reset_parameters(ccid_transport_t *handle);
