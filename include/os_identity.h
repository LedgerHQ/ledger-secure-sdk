/**
 * @file os_endorsement.h
 * @brief Header file containing all prototypes related to endorsement feature
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stddef.h>
#include "decorators.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Log out the device from the current identity.
 *
 * Once this function is called, the seed cannot be accessed anymore, until the the device is logged
 * in with `os_global_pin_check`.
 */
SYSCALL PERMISSION(APPLICATION_FLAG_GLOBAL_PIN) void sys_identity_log_out(void);

/**********************
 *      MACROS
 **********************/
