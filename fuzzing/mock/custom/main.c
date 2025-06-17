#include <stdint.h>
#include <string.h>

#include "os.h"
#include "io.h"

/**
 * Variable containing the length of the APDU response to send back.
 */
//static uint32_t G_output_len = 0;

/**
 * IO state (READY, RECEIVING, WAITING).
 */
//static io_state_e G_io_state;

bolos_ux_params_t G_ux_params;
