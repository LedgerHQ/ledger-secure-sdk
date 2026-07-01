#pragma once

#include "appflags.h"
#include "bolos_target.h"
#include "decorators.h"
#include "os_types.h"
#include "os_identity.h"

/* ----------------------------------------------------------------------- */
/* -                             PIN FEATURE                             - */
/* ----------------------------------------------------------------------- */

// Global PIN
#define DEFAULT_PIN_RETRIES 3

/*
 * @return BOLOS_UX_OK if pin validated
 */
SYSCALL bolos_bool_t os_global_pin_is_validated(void);

/**
 * Validating the pin also setup the identity linked with this pin (normal or alternate)
 * @return BOLOS_UX_OK if pin validated
 */
SYSCALL      PERMISSION(APPLICATION_FLAG_GLOBAL_PIN)
bolos_bool_t os_global_pin_check(unsigned char *pin_buffer PLENGTH(pin_length),
                                 unsigned char             pin_length);

/**
 * @brief Log out from current identity. Replaced by `sys_identity_log_out`.
 */
DEPRECATED SYSCALL PERMISSION(APPLICATION_FLAG_GLOBAL_PIN) void os_global_pin_invalidate(void);

/**
 * @brief Return the number of pin retries for the current identity.
 */
SYSCALL      PERMISSION(APPLICATION_FLAG_GLOBAL_PIN)
unsigned int os_global_pin_retries(void);

/**
 * @brief Check if main identity pin has been set.
 *
 * @return bolos_bool_t
 * @retval BOLOS_TRUE Main identity pin is set
 * @retval BOLOS_FALSE Main identity pin is not set
 */
SYSCALL
bolos_bool_t os_perso_is_pin_set(void);
